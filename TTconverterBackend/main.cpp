// Copyright © 2013 Tom Tondeur
// 
// This file is part of tt::Converter.
// 
// tt::Converter is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// tt::Converter is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with tt::Converter.  If not, see <http://www.gnu.org/licenses/>.

//Includes
//********
#include <tchar.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

#include "FileOutput.h"
#include "FbxFileReader.h"
#include "VertexAttributes.h"
#include "PhysxUserStream.h"

#include "pugiXML/pugixml.hpp"

#include <fbxsdk.h>

#include <PxVersionNumber.h>
#include <Px.h>
#include <PxFoundation.h>
#include <PxPhysics.h>
#include <cooking/PxCooking.h>

using namespace std;
using namespace pugi;
using namespace physx;

#ifdef UNICODE
	typedef wstring tstring;
#else
	typedef string tstring;
#endif

// Class definitions
//******************
struct Vertex{
	unsigned int iPosition;
	unsigned int iTexCoord;
	unsigned int iNormal;
	unsigned int iTangent;
	unsigned int iBinormal;
	unsigned int iVertexColor;
	unsigned int iAnimData;
	
	bool operator==(const Vertex& ref) const
	{
		return iPosition == ref.iPosition
			&& iTexCoord == ref.iTexCoord
			&& iNormal == ref.iNormal
			&& iTangent == ref.iTangent
			&& iBinormal == ref.iBinormal
			&& iVertexColor == ref.iVertexColor
			&& iAnimData == ref.iAnimData;
	}
};

struct AnimClip{
	string Name;
	float FramesPerSecond;
	map<double, vector<fbxsdk::FbxAMatrix> > TransformsAtTimeStamps;
};

enum class CollisionGeneration{
	Convex,
	Concave,
	None
};

//Forward declaration
//*******************
void ConvertFbxFile(string inFilename, string outFilename, vector<AnimClip>& animClips, CollisionGeneration generateCollision);

// Entrypoint
//***********
int main(int argc, char** argv) 
{
	vector<AnimClip> animClips;	

	//Try to load batch.xml
	xml_document doc;
	xml_parse_result result = doc.load_file("batch.xml");

	if(result.status != xml_parse_status::status_ok){
		cout << "Unable to read batch.xml.\n";
		system("pause");
		return 0;
	}

	//Get path name
	tstring oPath = doc.first_child().child(_T("Output")).child_value();
	string oPathName = string(oPath.begin(), oPath.end());

	//Read all fbx files
	for(auto& node : doc.first_child().children(_T("FbxFile")))
	{		
		//Get filename
		tstring iFile = node.child(_T("Filename")).child_value();
		string iFilename(iFile.begin(), iFile.end());

		//Get type of collision to generate
		CollisionGeneration generateCollision;
		auto colGenNode = node.child(_T("CollisionGeneration"));
		if(colGenNode.child_value() == _T("Concave"))
			generateCollision = CollisionGeneration::Concave;
		else if(colGenNode.child_value() == _T("Convex"))
			generateCollision = CollisionGeneration::Convex;
		else
			generateCollision = CollisionGeneration::None;

		//Read all animclips
		for(auto& animClipNode : node.children(_T("AnimClip")))
		{
			animClips.push_back(AnimClip());
			AnimClip& newClip = animClips.back();

			//Get clip name
			tstring clipName = animClipNode.child(_T("Name")).child_value();
			newClip.Name = string(clipName.begin(), clipName.end());
			
			//Get begin and end keyframe
			auto keyFrameNode = animClipNode.child(_T("Keyframes"));
			double firstKey = keyFrameNode.attribute(_T("Begin")).as_double();
			double lastKey  = keyFrameNode.attribute(_T("End")).as_double();

			//Get FPS
			newClip.FramesPerSecond = static_cast<float>(keyFrameNode.attribute(_T("FPS")).as_double());
			
			//Prepare animclip container, where we will store the sampled bone transforms
			double interval = 2.0/newClip.FramesPerSecond;
			for(double time = firstKey; time < lastKey + interval*0.5; time += interval)
				newClip.TransformsAtTimeStamps.insert(make_pair(time, vector<FbxAMatrix>() ) );
		}
		//Get filename to output (no extension)
		string oFilename = oPathName + iFilename.substr(iFilename.find_last_of('\\')+1, iFilename.find_last_of('.') - iFilename.find_last_of('\\') - 1);

		//Convert an fbx file
		ConvertFbxFile(iFilename, oFilename, animClips, generateCollision);
	}

	::system("pause");
    return 0;
}

void ConvertFbxFile(string inFilename, string outFilename, vector<AnimClip>& animClips, CollisionGeneration generateCollision)
{
	Mesh mesh;

	{
		//Get mesh from FileReader
		FbxFileReader fbxFile(inFilename);	
		mesh = fbxFile.GetMesh();

		std::cout << "\nProcessing FBX file " << inFilename << "...\n\n";
		//Extract vertex attributes and skeleton
		mesh.ExtractData();
		
		std::cout << "Done.\nOptimizing vertex attributes... ";
		//Remove duplicates and use indirect arrays
		mesh.Optimize();

		std::cout << "Done.\nExtracting bone transforms... ";
		//Get bone transforms
		for(auto& animClip : animClips)
			for(auto& transformAtTime : animClip.TransformsAtTimeStamps)
				transformAtTime.second = mesh.GetBoneTransforms(transformAtTime.first);
	}

	std::cout << "Done.\nChecking if vertices are linked to more than 4 bones... ";
	//Make sure none of the vertices is skinned to more than 4 bones
	for(auto& elem : mesh.BlendInformation.data)
		while(elem.BlendIndices.size() > 4)
		{
			elem.BlendIndices.pop_back();
			elem.BlendWeights.pop_back();
		}

	std::cout << "Done.\nBuilding vertex- and indexbuffers... ";
	// Construct vertexbuffer/indexBuffer
	vector<Vertex> vertexBuffer;
	vector<unsigned int> indexBuffer;
	for(unsigned int i=0; i < mesh.Positions.indices.size(); ++i){
		Vertex newVert;
		newVert.iPosition = mesh.Positions.indices[i];

		newVert.iTexCoord =		mesh.TexCoords.indices.empty()			? 0 : mesh.TexCoords.indices[i];
		newVert.iNormal =		mesh.Normals.indices.empty()			? 0 : mesh.Normals.indices[i];
		newVert.iTangent =		mesh.Tangents.indices.empty()			? 0 : mesh.Tangents.indices[i];
		newVert.iBinormal =		mesh.Binormals.indices.empty()			? 0 : mesh.Binormals.indices[i];
		newVert.iVertexColor =	mesh.Colors.indices.empty()				? 0 : mesh.Colors.indices[i];
		newVert.iAnimData =		mesh.BlendInformation.indices.empty()	? 0 : mesh.BlendInformation.indices[i];

		auto it = find(vertexBuffer.begin(), vertexBuffer.end(), newVert);

		if(it==vertexBuffer.end()){
			vertexBuffer.push_back(newVert);
			it = vertexBuffer.end() - 1;
		}

		indexBuffer.push_back(it-vertexBuffer.begin());
	}

	std::cout << "Done.\nWriting mesh data... ";
	//Write a binary file containing all of the mesh & skeleton data

	unsigned int version=1, nrOfUVChannels=mesh.Normals.data.empty() ?0:1, vertexFormat=0;

	//Build vertex format
	vertexFormat |= mesh.Normals.data.empty()			? 0 : 1 << 0;
	vertexFormat |= mesh.Tangents.data.empty()			? 0 : 1 << 1;
	vertexFormat |= mesh.Binormals.data.empty()			? 0 : 1 << 2;
	vertexFormat |= mesh.Colors.data.empty()			? 0 : 1 << 3;
	vertexFormat |= mesh.BlendInformation.data.empty()	? 0 : 1 << 4;
		
	//Create an output file
	BinaryWriter oFile(outFilename + ".ttmesh");

	oFile.Write<unsigned short>(version); //version number
	oFile.Write<unsigned char>(vertexFormat); // vertex format
	oFile.Write<unsigned char>(nrOfUVChannels); // nr of texcoord channels
	
	oFile.Write<unsigned int>(mesh.Positions.data.size()); // nr of positions

	// nr of texcoords
	if(nrOfUVChannels > 0)
		oFile.Write<unsigned int>(mesh.TexCoords.data.size());
	
	// nr of normals
	if(vertexFormat & 1 << 0)
		oFile.Write<unsigned int>(mesh.Normals.data.size()); 
	
	// nr of tangents
	if(vertexFormat & 1 << 1)
		oFile.Write<unsigned int>(mesh.Tangents.data.size()); 
	
	// nr of binormals
	if(vertexFormat & 1 << 2)
		oFile.Write<unsigned int>(mesh.Binormals.data.size()); 
	
	// nr of vertex colors
	if(vertexFormat & 1 << 3)
		oFile.Write<unsigned int>(mesh.Colors.data.size()); 
	
	//nr of blendweights/-indices
	if(vertexFormat & 1 << 4)
		oFile.Write<unsigned int>(mesh.BlendInformation.data.size());
	
	oFile.Write<unsigned int>(vertexBuffer.size()); // nr of vertices
	oFile.Write<unsigned int>(indexBuffer.size()); // nr of indices

	for(auto& elem : mesh.Positions.data) //positions
		oFile.Write<FbxVector4>(elem);
	
	for(auto& elem : mesh.TexCoords.data) //texCoords
		oFile.Write<FbxVector2>(FbxVector2(elem.mData[0],1-elem.mData[1]));
	
	for(auto& elem : mesh.Normals.data) //normals
		oFile.Write<FbxVector4>(elem);

	for(auto& elem : mesh.Tangents.data) //tangents
		oFile.Write<FbxVector4>(elem);

	for(auto& elem : mesh.Binormals.data) //binormals
		oFile.Write<FbxVector4>(elem);
	
	for(auto& elem : mesh.Colors.data) //vertex colors
		oFile.Write<FbxColor>(elem);
	
	for(auto& elem : mesh.BlendInformation.data){ 
		//blend indices
		oFile.Write<unsigned int>(elem.BlendIndices.size() );
		for(auto index : elem.BlendIndices)
			oFile.Write<unsigned int>(index);

		//blend weights
		for(auto weight : elem.BlendWeights)
			oFile.Write<float>(static_cast<float>(weight) );
	}

	//Vertex buffer
	for(auto& vertex : vertexBuffer){
		oFile.Write<unsigned int>(vertex.iPosition);
		if(nrOfUVChannels > 0)
			oFile.Write<unsigned int>(vertex.iTexCoord);
		if(vertexFormat & 1 << 0)
			oFile.Write<unsigned int>(vertex.iNormal);
		if(vertexFormat & 1 << 1)
			oFile.Write<unsigned int>(vertex.iTangent);
		if(vertexFormat & 1 << 2)
			oFile.Write<unsigned int>(vertex.iBinormal);
		if(vertexFormat & 1 << 3)
			oFile.Write<unsigned int>(vertex.iVertexColor);
		if(vertexFormat & 1 << 4)
			oFile.Write<unsigned int>(vertex.iAnimData);
	}

	//Index buffer
	for(auto index : indexBuffer)
		oFile.Write<unsigned int>(index);

	std::cout << "Done.\nWriting skeleton data... ";
	//Bones (#, names, bindposes)
	oFile.Write<unsigned int>( mesh.Skeleton.size() );
	for(auto& bone : mesh.Skeleton){
		oFile.Write<std::string>(bone.Name);
		oFile.Write<FbxAMatrix>(bone.BindPose);
	}
	std::cout << "Done.\nWriting bone animations... ";
	//animClips (#, name, fps, nrOfKeys, keyTime0, boneTransforms0, keyTime1, boneTransforms1...)
	oFile.Write<unsigned int>( animClips.size() );
	for(auto& animClip : animClips){
		oFile.Write<std::string>(animClip.Name);
		oFile.Write<float>(animClip.FramesPerSecond);
		oFile.Write<unsigned int>( animClip.TransformsAtTimeStamps.size() );

		for(auto& animKey : animClip.TransformsAtTimeStamps){
			oFile.Write<float>(static_cast<float>(animKey.first) );
			
			for(auto& boneTransform : animKey.second)
				oFile.Write<FbxAMatrix>(boneTransform);
		}
	}

	//Check if we need to generate a collision mesh
	if(generateCollision == CollisionGeneration::None)
	{
		std::cout << "Done.\n\nOperation succeeded!\n\n";
		return;
	}

	std::cout << "Done.\nWriting PhysX data... ";
	//Initialize cooker
	PxCookingParams params{ PxTolerancesScale() };
	PxCooking* pCooker = PxCreateCooking(PX_PHYSICS_VERSION, PxGetFoundation(), params);

	//Build a vertex buffer for PhysX (containing only vertex positions), also copy index buffer, casting to PxU32

	unsigned int nrOfVerts = mesh.Positions.data.size();
	unsigned int nrOfIndices = mesh.Positions.indices.size();
	PxVec3* pVertices	= new PxVec3[nrOfVerts];
	PxU32* pIndices		= new PxU32[nrOfIndices];

	for(unsigned int i=0; i<nrOfVerts; ++i){
		auto pos = mesh.Positions.data[i];
		pVertices[i] = PxVec3( static_cast<PxReal>(pos.mData[0]), static_cast<PxReal>(pos.mData[1]), static_cast<PxReal>(pos.mData[2]) );
	}

	for(unsigned int i=0; i<nrOfIndices; ++i)
		pIndices[i] = static_cast<PxU32>(mesh.Positions.indices[i]);
	
	PxTriangleMeshDesc triMeshDesc;
	PxConvexMeshDesc convexMeshDesc;

	//Build physical model 
	switch(generateCollision){
	case CollisionGeneration::Concave: 
		//Fill desc
		triMeshDesc.points.count		= nrOfVerts;
		triMeshDesc.triangles.count		= nrOfIndices / 3;
		triMeshDesc.points.stride		= sizeof(PxVec3);
		triMeshDesc.triangles.stride	= 3 * sizeof(PxU32);
		triMeshDesc.points.data			= pVertices;
		triMeshDesc.triangles.data		= pIndices;
		//Cook
		pCooker->cookTriangleMesh(triMeshDesc, UserStream((outFilename + ".ttcol").c_str(), false));
		break;
	case CollisionGeneration::Convex:
		//Fill desc
		convexMeshDesc.points.count		= nrOfVerts;    
		convexMeshDesc.triangles.count	= nrOfIndices / 3;    
		convexMeshDesc.points.stride	= sizeof(PxVec3);    
		convexMeshDesc.triangles.stride = 3*sizeof(PxU32);    
		convexMeshDesc.points.data		= pVertices;
		convexMeshDesc.triangles.data	= pIndices;    
		convexMeshDesc.flags.set(PxConvexFlag::Enum::eCOMPUTE_CONVEX);
		//Cook
		pCooker->cookConvexMesh(convexMeshDesc, UserStream( (outFilename + ".ttcol").c_str(), false));
		break;
	};

	//Stop cooking
	pCooker->release();

	std::cout << "Done.\n\nOperation succeeded!\n\n";
}

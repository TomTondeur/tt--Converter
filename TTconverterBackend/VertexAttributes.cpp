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

#include "VertexAttributes.h"
#include "FileOutput.h"
			
#include <map>
#include <iostream>

using namespace std;

//Constructor & Destructor
Mesh::Mesh(FbxMesh* _pMesh):pMesh(_pMesh){}
Mesh::Mesh(void):pMesh(nullptr){}

//Get mesh node name
string Mesh::GetName(void)
{
	if(!pMesh)
		throw exception("Failure to extract data: Mesh object is uninitialized");

	return string(pMesh->GetName() );
}

//Check if this mesh is deformed
bool Mesh::ContainsAnimationData(void)
{
	if(!pMesh)
		throw exception("Failure to extract data: Mesh object is uninitialized");

	return pMesh->GetDeformerCount() > 0;
}

//Sample bone transforms at a specified point in time
vector<FbxAMatrix> Mesh::GetBoneTransforms(double time) const
{
	vector<FbxAMatrix> out;
	FbxTime fbxTime;
	fbxTime.SetFrame(FbxLongLong(time) );
	
	for(auto& bone : Skeleton)
		out.push_back( bone.pFbxNode->EvaluateGlobalTransform(fbxTime) );

	return out;
}

//Extract vertex attributes from the fbx sdk
void Mesh::ExtractData(void)
{		
	if(!pMesh)
		throw exception("Failure to extract data: Mesh object is uninitialized");
	
	cout << "Extracting vertex attributes... ";
	//Copy vertex attribute arrays into vectors of our own
	unsigned int layerCount = pMesh->GetLayerCount();
	unsigned int triCount = pMesh->GetPolygonCount();
	for (unsigned int layer=0; layer<layerCount; ++layer){
		auto pLayer = pMesh->GetLayer(layer);			
	
		TexCoords.ExtractData(	pMesh, pLayer->GetUVs() );
		Normals.ExtractData(	pMesh, pLayer->GetNormals() );
		Tangents.ExtractData(	pMesh, pLayer->GetTangents() );
		Binormals.ExtractData(	pMesh, pLayer->GetBinormals() );
		Colors.ExtractData(		pMesh, pLayer->GetVertexColors() );
	}
	
	map<unsigned int, BlendInfo> blendInfoPerVertexIndex;
	
	cout << "Done.\nBuilding skeleton... ";
	//Get skeleton data
	unsigned int nrOfDeformers = pMesh->GetDeformerCount();
	for(unsigned int iDeformer=0; iDeformer < nrOfDeformers; ++iDeformer){
		auto pSkin = reinterpret_cast<FbxSkin*>( pMesh->GetDeformer(iDeformer, FbxDeformer::eSkin) );

		if(!pSkin)
			continue;

		//pSkin->GetSkinningType() eLinear / eRigid, eDualQuaternion, eBlend
		//FbxCluster::ELinkMode clusterMode = ((FbxSkin*)pMesh->GetDeformer(0, FbxDeformer::eSkin))->GetCluster(0)->GetLinkMode(); eAdditive, eNormalize, eTotalOne

		unsigned int nrOfClusters = pSkin->GetClusterCount();
		for(unsigned int iCluster=0; iCluster < nrOfClusters; ++iCluster){
			auto pCluster = pSkin->GetCluster(iCluster);
			
			//Add bone to skeleton
			Skeleton.push_back(Bone());
			auto& newBone = Skeleton.back();
			auto pNode = pCluster->GetLink();

			pCluster->GetTransformLinkMatrix(newBone.BindPose);
			newBone.Name = pNode->GetName();
			newBone.pFbxNode = pNode;
			newBone.pCluster = pCluster;
				
			//Get blend index & weight per vertex
			unsigned int nrOfIndices = pCluster->GetControlPointIndicesCount();
			for(unsigned int i=0; i<nrOfIndices; ++i){
				unsigned int ptIndex = pCluster->GetControlPointIndices()[i];
				double weight = pCluster->GetControlPointWeights()[i];

				auto it = blendInfoPerVertexIndex.find(ptIndex);

				if(it == blendInfoPerVertexIndex.end())
					it = blendInfoPerVertexIndex.insert(make_pair(ptIndex, BlendInfo() ) ).first;
				
				it->second.BlendIndices.push_back(iCluster);
				it->second.BlendWeights.push_back(weight);
			}
		}
	}
	//Allocate necessary data
	Positions.data.reserve(triCount*3);
	if(nrOfDeformers > 0)
		BlendInformation.data.reserve(triCount*3);
	
	//Get vertex positions and link them up to bones if necessary
	for (unsigned int iTri=0; iTri<triCount; ++iTri){
		for (unsigned int iVert=0; iVert<3; ++iVert){
			int cpIndex = pMesh->GetPolygonVertex(iTri,iVert);
			Positions.data.push_back( pMesh->GetControlPointAt(cpIndex) );
			
			if(nrOfDeformers > 0){
				auto& blendInfo = blendInfoPerVertexIndex.find(cpIndex)->second;
				BlendInformation.data.push_back(move(blendInfo));
			}
		}
	}
}

void Mesh::Optimize(void)
{
	Positions.Optimize();
	TexCoords.Optimize();
	Normals.Optimize();
	Tangents.Optimize();
	Binormals.Optimize();
	Colors.Optimize();
	BlendInformation.Optimize();
}

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

#pragma once

#include <fbxsdk.h>
#include <vector>

struct BlendInfo{
	std::vector<unsigned int>	BlendIndices;
	std::vector<double>			BlendWeights;

	bool operator==(const BlendInfo& other)
	{
		return BlendIndices == other.BlendIndices && BlendWeights == other.BlendWeights;
	}

	BlendInfo operator=(BlendInfo&& src)
	{
		BlendInfo newObj;
		newObj.BlendIndices = std::move(BlendIndices);
		newObj.BlendWeights = std::move(BlendWeights);
		return newObj;
	}
};

template<typename T>
//Generic class to store vertex attributes (texcoords, normals...)
struct VertexAttribute{
	std::vector<T> data;
	std::vector<unsigned int> indices;

	const T& GetRefAt(unsigned int vertexIndex)
	{
		return data.at(indices.at(vertexIndex));
	}

	void ExtractData(FbxMesh* pMesh, FbxLayerElementTemplate<T>* pLayerElement)
	{
		if(!pLayerElement) //Input validation
			return;
		
		//Get internal data array
		const auto dataArr = pLayerElement->GetDirectArray();
		const unsigned int nrOfEntries = dataArr.GetCount();
		
		//Copy data array to our vector
		data.reserve(nrOfEntries); //Avoid re-allocating
		for(unsigned int i=0; i < nrOfEntries; ++i)
			data.push_back(dataArr.GetAt(i));

		//Check if the fbx sdk uses an internal index array
		const auto referenceMode = pLayerElement->GetReferenceMode();
		if(referenceMode == FbxLayerElement::eDirect)
			return;
		
		const auto mappingMode = pLayerElement->GetMappingMode();
		const unsigned int nrOfTriangles = pMesh->GetPolygonCount();
		
		auto& idxArr = pLayerElement->GetIndexArray();
		
		//Loop through all vertices and get the index associated with them
		for(unsigned int iTri=0; iTri < nrOfTriangles; ++iTri){
			for(unsigned int iVert=0; iVert<3; ++iVert){

				switch(mappingMode){
					case FbxLayerElement::eByControlPoint:
						switch(referenceMode)
						{
							case FbxLayerElement::eIndexToDirect:
								indices.push_back(idxArr[pMesh->GetPolygonVertex(iTri,iVert)]);
								break;
							default:
								throw std::exception("Invalid reference mode");
						}
						break;
					case FbxLayerElement::eByPolygonVertex: 
						switch(referenceMode)
						{
							case FbxLayerElement::eIndexToDirect:
								indices.push_back(idxArr[(3*iTri + iVert)]);
								break;
							default:
								throw std::exception("Invalid reference mode");
						}
						break;
					default:
						throw std::exception("Invalid mapping mode");
				}
			}
		}
	}

	void Optimize(void)
	{
		//No data or already optimized => skip
		if(data.empty() || !indices.empty())
			return;

		//Prepare index array & unique data array
		const unsigned int nrOfIndices = data.size();
		std::vector<T> uniqueData;
		indices.reserve(nrOfIndices);

		for(auto& elem : data)
		{
			//See if we already have an attribute with this value
			auto it = find(uniqueData.begin(), uniqueData.end(), elem);
		
			//New unique attribute => add it to the unique data array
			if(it == uniqueData.end()){ 
				uniqueData.push_back(elem);
				it = uniqueData.end() - 1;
			}
			
			//Add index to index array
			indices.push_back(it - uniqueData.begin());
		}

		//Replace data by unique data
		data = std::move(uniqueData);
	}
};

struct Bone{
	std::string Name;
	fbxsdk_2014_1::FbxAMatrix BindPose;
	fbxsdk_2014_1::FbxNode* pFbxNode;
	fbxsdk_2014_1::FbxCluster* pCluster;
};

struct Mesh
{
	//Contructor & destructor
	Mesh(FbxMesh* _pMesh);
	Mesh(void);
	
	//Extract vertex attributes from the fbx sdk
	void ExtractData(void);
	
	//Optimize vertex attributes for space (warning: slow)
	void Optimize(void);
	
	//Get mesh node name
	std::string GetName(void);
	
	//Sample bone transforms at a specified point in time
	std::vector<FbxAMatrix> GetBoneTransforms(double time) const;
	
	//Check if this mesh is deformed
	bool ContainsAnimationData(void);

	VertexAttribute<FbxVector4> Positions;
	VertexAttribute<FbxVector2> TexCoords;
	VertexAttribute<FbxVector4> Normals;
	VertexAttribute<FbxVector4> Tangents;
	VertexAttribute<FbxVector4> Binormals;
	VertexAttribute<FbxColor>	Colors;
	VertexAttribute<BlendInfo>	BlendInformation;

	std::vector<Bone> Skeleton;

private:
	FbxMesh* pMesh;
};
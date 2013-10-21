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

#include "FbxFileReader.h"
#include <algorithm>

using namespace std;

FbxFileReader::FbxFileReader(const string& filename) : m_pScene(nullptr), m_pSdkManager(nullptr)
{
    // Initialize the resources needed to import fbx files
    m_pSdkManager = FbxManager::Create();
    
	FbxIOSettings* pIOSettings = FbxIOSettings::Create(m_pSdkManager, IOSROOT);
    m_pSdkManager->SetIOSettings(pIOSettings);
	
	FbxImporter* pImporter = FbxImporter::Create(m_pSdkManager,"");
    if( !pImporter->Initialize(filename.c_str(), -1, pIOSettings) )
        throw exception("Failed to initialize FbxImporter.");
    
    // Prepare a scene to contain the fbx data
    m_pScene = FbxScene::Create(m_pSdkManager,"myScene");
    pImporter->Import(m_pScene);

    // Done importing
    pImporter->Destroy();

	//Check if the scene contains a root node
    if( !m_pScene->GetRootNode() )
        throw std::exception("No RootNode could be found.");
}

FbxFileReader::~FbxFileReader(void)
{

}

//Methods

Mesh& FbxFileReader::GetMesh(void)
{
	if(m_Meshes.empty())
		ReadMeshes();

	return m_Meshes[0];
}
	
Mesh& FbxFileReader::GetMesh(const std::string& nodeName)
{
	if(m_Meshes.empty())
		ReadMeshes();

	auto it = find_if(m_Meshes.begin(), m_Meshes.end(), [&](Mesh& meshRef){
		return meshRef.GetName() == nodeName;
	});

	if(it == m_Meshes.end())
		throw exception( ("Failed to find a mesh with the name " + nodeName).c_str() );

	return *it;
}

void FbxFileReader::ReadMeshes(void)
{
    FbxNode* pRootNode = m_pScene->GetRootNode();
	
	// Search hierarchy for mesh node
	int nrOfChildren = pRootNode->GetChildCount();
	for(int i=0; i<nrOfChildren; ++i)
		ReadMeshesRecursive(pRootNode);
	
	if(m_Meshes.empty())
		throw exception("Failed to find mesh data.");
}

//Recursive function going through the entire FBX node hierarchy looking for mesh nodes
void FbxFileReader::ReadMeshesRecursive(FbxNode* pNode)
{
	FbxNodeAttribute* pAttribute = pNode->GetNodeAttribute();

	if(pAttribute && pAttribute->GetAttributeType() == FbxNodeAttribute::eMesh)
		m_Meshes.push_back( Mesh( pNode->GetMesh() ) );

	//Search children
	unsigned int nrOfChildren = pNode->GetChildCount();
	for(unsigned int i=0; i < nrOfChildren; ++i)
		ReadMeshesRecursive(pNode->GetChild(i));
}
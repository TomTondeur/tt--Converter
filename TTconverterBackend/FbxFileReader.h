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

#include <string>
#include <fbxsdk.h>
#include "VertexAttributes.h"

class FbxFileReader
{
public:
	// * Imports a .fbx file and creates a scene to hold its contents.
	FbxFileReader(const std::string& filename);
	virtual ~FbxFileReader(void);

	//Methods
	
	// * Retrieves the first MeshNode found in the scene. 
	// * Throws exception no meshes are found.
	Mesh& GetMesh(void);
	
	// * Retrieves the MeshNode with the given name. 
	// * Throws exception when mesh is not found.
	Mesh& GetMesh(const std::string& filename);
	
private:
	//Datamembers
	FbxManager* m_pSdkManager;
	FbxScene* m_pScene;	
	std::vector<Mesh> m_Meshes;

	void ReadMeshes(void);

	//Recursive function going through the entire FBX node hierarchy looking for mesh nodes
	void ReadMeshesRecursive(FbxNode* pNode);

	//Disabling default copy constructor & assignment operator
	FbxFileReader(const FbxFileReader& src);
	FbxFileReader& operator=(const FbxFileReader& src);
};

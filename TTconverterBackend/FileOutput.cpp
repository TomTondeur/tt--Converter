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

#include "FileOutput.h"

//Create matrix to transform from Max to DX axis system
FbxAMatrix BinaryWriter::s_MaxToDxMat = FbxAMatrix(FbxVector4(0,0,0,1), FbxVector4(90,0,0,1), FbxVector4(1,1,-1,1));

//Constructor & Destructor
//************************

BinaryWriter::BinaryWriter(const std::string& filename):oFile(filename, std::ios::binary)
{}

BinaryWriter::~BinaryWriter(void)
{
	oFile.close();
}

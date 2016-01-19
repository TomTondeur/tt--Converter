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
#include <fstream>
#include "fbxsdk.h"

class BinaryWriter final
{
public:
	//Create new writer
	BinaryWriter(const std::string& filename);
	~BinaryWriter(void); //Default destructor

	template<typename T>
	//Write data to a binary file
	void Write(const T& val)
	{
		WriteImpl<T>::execute(val, oFile);
	}

private:
	//filestream
	std::ofstream oFile;

	//Matrix that can be used to transform matrices and vectors from the 3ds Max axis system to the DirectX axis system (flips z and rotates around x)
	static FbxAMatrix s_MaxToDxMat;
	
	//Disabling copy constructor & assignment operator
	BinaryWriter(const BinaryWriter& src);
	BinaryWriter& operator=(const BinaryWriter& src);

	template<typename T>
	//Default binary serialization of any data type
	struct WriteImpl
	{
		static void execute(const T& val, std::ofstream& oFile)
		{
			oFile.write(reinterpret_cast<const char*>(&val), sizeof(T));
		}
	};
	
	//WriteImpl specializations
	//*************************

	template<>
	struct BinaryWriter::WriteImpl<std::string>
	{ 
		static void execute(const std::string& str, std::ofstream& oFile)
		{
			auto strLen = (char)str.size();
			oFile.write(&strLen, 1);
			oFile.write(str.c_str(), strLen);
		}
	};

	template<>
	struct BinaryWriter::WriteImpl<FbxAMatrix>
	{ 
		static void execute(const FbxAMatrix& mat, std::ofstream& oFile)
		{
			FbxAMatrix tmp = s_MaxToDxMat * mat;
			
			for(unsigned int row=0; row<4; ++row)
				for(unsigned int col=0; col<3; ++col)
					WriteImpl<float>::execute(static_cast<float>( tmp.Get(row,col) ), oFile);
		}
	};

	template<>
	struct BinaryWriter::WriteImpl<FbxVector2>
	{
		static void execute(const FbxVector2& vec, std::ofstream& oFile)
		{
			WriteImpl<float>::execute(static_cast<float>(vec.mData[0]), oFile);
			WriteImpl<float>::execute(static_cast<float>(vec.mData[1]), oFile);
		}
	};

	template<>
	struct BinaryWriter::WriteImpl<FbxVector4>
	{ 
		static void execute(const FbxVector4& vec, std::ofstream& oFile)
		{
			FbxAMatrix tmpMat(vec, FbxVector4(0,0,0,1), FbxVector4(1,1,1,1));
			tmpMat = s_MaxToDxMat * tmpMat;
			WriteImpl<float>::execute(static_cast<float>(tmpMat.GetT().mData[0]), oFile);
			WriteImpl<float>::execute(static_cast<float>(tmpMat.GetT().mData[1]), oFile);
			WriteImpl<float>::execute(static_cast<float>(tmpMat.GetT().mData[2]), oFile);
		}
	};

	template<>
	struct BinaryWriter::WriteImpl<FbxColor>
	{ 
		static void execute(const FbxColor& col, std::ofstream& oFile)
		{
			WriteImpl<float>::execute(static_cast<float>(col.mRed)	, oFile);
			WriteImpl<float>::execute(static_cast<float>(col.mGreen), oFile);
			WriteImpl<float>::execute(static_cast<float>(col.mBlue)	, oFile);
			WriteImpl<float>::execute(static_cast<float>(col.mAlpha), oFile);
		}
	};
};

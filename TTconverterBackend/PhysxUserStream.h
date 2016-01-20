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

#include "PxAssert.h"
#include "PxSimpleTypes.h"
#include "PxIO.h"

//Classes implementing the PhysX NxStream interface so we can output files through the PhysX API

class UserStream : public physx::PxInputStream, public physx::PxOutputStream
{
public:
	UserStream(const char* filename, bool load);
	virtual ~UserStream();

	virtual physx::PxU32 write(const void* src, physx::PxU32 count) override;
	virtual physx::PxU32 read(void* dest, physx::PxU32 count) override;

	FILE* fp;
};

class MemoryWriteBuffer : public physx::PxOutputStream
{
public:
	MemoryWriteBuffer();
	virtual ~MemoryWriteBuffer();
	void clear();

	virtual physx::PxU32 write(const void* src, physx::PxU32 count) override;

	physx::PxU32			currentSize;
	physx::PxU32			maxSize;
	physx::PxU8*			data;
};

class MemoryReadBuffer : public physx::PxInputStream
{
public:
	MemoryReadBuffer(physx::PxU8* data);
	virtual	~MemoryReadBuffer();

	virtual physx::PxU32 read(void* dest, physx::PxU32 count) override;

	physx::PxU8* buffer;
};

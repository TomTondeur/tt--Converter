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

#include <stdio.h>
#include "PxPhysics.h"
#include "PhysxUserStream.h"

using namespace physx;

//UserStream
//**********

UserStream::UserStream(const char* filename, bool load) : fp(NULL)
{
	fopen_s(&fp, filename, load ? "rb" : "wb");
}

UserStream::~UserStream()
{
	if(fp)	fclose(fp);
}

// Loading API
PxU32 UserStream::write(const void* src, physx::PxU32 count)
{
	return fwrite(src, count, 1, fp);
}

// Saving API
PxU32 UserStream::read(void* dest, physx::PxU32 count)
{
	return fread(dest, count, 1, fp);
}

//MemoryWriteBuffer
//*****************

MemoryWriteBuffer::MemoryWriteBuffer() : currentSize(0), maxSize(0), data(NULL)
{}

MemoryWriteBuffer::~MemoryWriteBuffer()
{
	delete data;
	data = 0;
}

void MemoryWriteBuffer::clear()
{
	currentSize = 0;
}

physx::PxU32 MemoryWriteBuffer::write(const void* src, physx::PxU32 count)
{
	PxU32 expectedSize = currentSize + count;
	if(expectedSize > maxSize)
	{
		maxSize = expectedSize + 4096;

		PxU8* newData = new PxU8[maxSize];
		PX_ASSERT(newData!=NULL);

		if(data)
		{
			memcpy(newData, data, currentSize);
			delete[] data;
		}
		data = newData;
	}
	memcpy(data+currentSize, src, count);
	currentSize += count;
	return count;
}

//MemoryReadBuffer
//****************

MemoryReadBuffer::MemoryReadBuffer(PxU8* data) : buffer(data)
{}

MemoryReadBuffer::~MemoryReadBuffer() // We don't own the data => no delete
{}
PxU32 MemoryReadBuffer::read(void* dest, PxU32 count)
{
	memcpy(dest, buffer, count);
	buffer += count;
	return count;
}

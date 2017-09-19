/*
MIT License

Copyright (c) 2017 Srijan Kumar Sharma

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include "DDSLoader.h"
#include <string>
#include <fstream>
#include <DirectXTex.h>

DDS::DDS() : data(0), dataSize(0)
{
	memset(&header, 0, sizeof(header));
}

DDS::DDS(uint32_t flags, uint32_t height, uint32_t width, uint32_t depth, uint32_t mip, uint32_t compresion, char* _data, uint32_t _size)
{
	memset(&header, 0, sizeof(DDS_Header));
	header.dwSize = sizeof(DDS_Header);
	header.dwFlags = flags | 0x1007;
	header.dwHeight = height;
	header.dwWidth = width;
	header.dwPitchOrLinearSize = 0;
	header.dwDepth = depth;
	header.dwMipMapCount = mip;
	//header.dwReserved1[11];
	header.ddspf.dwSize = sizeof(DDS_PIXELFORMAT);
	header.ddspf.dwFlags = DDPF_FOURCC;
	header.ddspf.dwFourCC = compresion;
	header.dwCaps = 0x1000 | ((mip > 1) ? 0x400008 : 0x0);
	header.dwCaps2 = 0;
	header.dwCaps3 = 0;
	header.dwCaps4 = 0;
	//header.dwReserved2;
	dataSize = _size;
	data = new char[dataSize];
	memcpy(data, _data, _size);
}


DDS::~DDS(void)
{
	delete data;
}

bool DDS::deserialization(string fileName)
{
	ifstream ddsFile(fileName, ios_base::binary);
	if (!ddsFile.is_open())
	{
		return false;
	}
	char magic[5] = { 0 };
	ddsFile.read(magic, 4);
	ddsFile.read((char*)&header, sizeof(header));
	dataSize = header.dwWidth * header.dwHeight * ((header.dwMipMapCount > 0) ? header.dwMipMapCount : 1);
	data = new char[dataSize];
	ddsFile.read(data, dataSize);
	return true;
}

void DDS::serialization(string fileName)
{
	fstream ddsFile(fileName.c_str(), ios_base::out | ios_base::binary);
	if (!ddsFile.is_open())
		return exit(errno);
	ddsFile.seekg(0);
	ddsFile.seekp(0);
	ddsFile << DDS_MAGIC;
	ddsFile.write((char*)&header, sizeof(DDS_Header));
	ddsFile.write(data, dataSize);
	ddsFile.close();
}

void DDS::writeToMemory(char * pData)
{
	memcpy(pData, DDS_MAGIC, strlen(DDS_MAGIC));
	memcpy(pData + strlen(DDS_MAGIC), (char*)&header, sizeof(DDS_Header));
	memcpy(pData + strlen(DDS_MAGIC) + sizeof(DDS_Header), data, dataSize);
}

uint32_t DDS::getDataSize()
{
	return dataSize;
}

size_t DDS::getDDSSize()
{
	return dataSize + strlen(DDS_MAGIC) + sizeof(DDS_Header);
}

char* DDS::getData()
{
	return data;
}

uint32_t DDS::getFormat()
{
	return header.dwFlags;
}

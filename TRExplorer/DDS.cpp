#include "DDS.h"
#include <string>
#include <fstream>

DDS::DDS() : data(0), dataSize(0)
{
	memset(&header, 0, sizeof(header));
}

DDS::DDS(uint32_t flags, uint32_t height, uint32_t width, uint32_t depth, uint32_t mip, uint32_t compresion, char* _data, uint32_t _size)
{
	memset(&header, 0, sizeof(DDS_Header));
	header.dwSize = sizeof(DDS_Header);
	header.dwFlags = flags;
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
	memcpy(data, _data, dataSize);
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
	ddsFile << "DDS ";
	ddsFile.write((char*)&header, sizeof(DDS_Header));
	ddsFile.write(data, dataSize);
	ddsFile.close();
}

uint32_t DDS::getDataSize()
{
	return dataSize;
}

char* DDS::getData()
{
	return data;
}
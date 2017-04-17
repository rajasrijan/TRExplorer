#ifndef TIGER_H
#define TIGER_H

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <stdint.h>
#include <memory>
#include <map>
#include <algorithm>
#include "cdrm.h"

#define ALIGN_TO(address,alignment) (((address+alignment-1)/alignment)*alignment)
#define TIGER_FILEID_MASK (0x7FF)

using namespace std;

extern map<uint32_t, void*> tigerPtrMap;

struct unknown_header1
{
	uint32_t DataSize;
	uint32_t ddsFlags;
	uint32_t flags;
	uint32_t ID;
	uint32_t u3;
};

class unknown_header2
{
public:
	uint32_t u1;
	uint32_t offset;
	uint32_t size;
	uint32_t u4;
	void save(fstream* filestream)
	{
		filestream->write((char*)this, sizeof(unknown_header2));
	}
	int setSize(size_t newSize);
	CDRM_Header* getCDRMPtr()
	{
		if (tigerPtrMap.find(offset & TIGER_FILEID_MASK) != tigerPtrMap.end())
		{
			return (CDRM_Header*)((char*)(tigerPtrMap[offset & TIGER_FILEID_MASK]) + (offset&(~TIGER_FILEID_MASK)));
		}
		else
		{
			fprintf(stderr, "Error occured trying to read offset [%#X]. Not implimented\n", offset);
			return nullptr;
		}
	}
	uint32_t getCDRMDataSize()
	{
		return size;
	}
	CDRM_BlockFooter* getCDRMFooter();
};

class DRM_Header
{
public:
	uint32_t version; // 0x16 for TR9
	uint32_t strlen1, strlen2;
	uint32_t unknown[3]; // all zero? must check all DRM files
	uint32_t SectionCount;
	uint32_t UnknownCount;
	DRM_Header()
	{}
	~DRM_Header()
	{}
	void load(iostream *dataStream)
	{
		dataStream->read((char*)this, sizeof(DRM_Header));
	}
	void printHeader()
	{
		cout << "DRM Header\n";
		cout << "version\t\t:" << hex << version << dec;
		cout << "\nstrlen1\t\t:" << strlen1;
		cout << "\nstrlen2\t\t:" << strlen2;
		cout << "\nunknown\t\t:" << unknown[0] << "," << unknown[1] << "," << unknown[2];
		cout << "\nSectionCount\t:" << SectionCount;
		cout << "\nUnknownCount\t:" << UnknownCount;
		cout << endl;
	}

	vector<unknown_header2> it(iostream *dataStream, int dummy = 0)
	{
		vector<unknown_header2> vec(SectionCount);
		dataStream->read((char*)vec.data(), SectionCount * sizeof(unknown_header2));
		return vec;
	}
	unknown_header2* getUnknownHeaders2()
	{
		return (unknown_header2*)((char*)this + sizeof(DRM_Header) + ((SectionCount * sizeof(unknown_header1)) + strlen1 + strlen2));
	}
	uint32_t getUnknownHeaders2Count()
	{
		return SectionCount;
	}
};

//DRM end

class StringTable_Header
{
public:
	uint32_t local; // 0x00 for english
	uint32_t unknownCount;
	uint32_t tableRange;
	StringTable_Header()
	{
		printHeader();
	}
	~StringTable_Header()
	{

	}
	void printHeader()
	{
		cout << "StringTable Header\n";
		cout << "\nLocal\t\t:" << local;
		cout << "\nunknownCount\t:" << unknownCount;
		cout << "\ntableRange\t:" << tableRange;
		cout << endl;
	}
	uint32_t* getStringIndicesPtr();
	uint32_t getStringIndicesCount();
};

// TIGER START
struct element
{
	uint32_t nameHash;
	uint32_t Local;
	uint32_t size;
	uint32_t offset;
	void* getDataPtr()
	{
		if (tigerPtrMap.find(offset & TIGER_FILEID_MASK) != tigerPtrMap.end())
		{
			return (void*)((char*)(tigerPtrMap[offset & TIGER_FILEID_MASK]) + (offset&(~TIGER_FILEID_MASK)));
		}
		else
		{
			printf("Error occured trying to read offset [%#X]", offset);
			exit(1);
			return NULL;
		}
	}
};

class file_header
{
public:
	uint32_t magic;
	uint32_t version;
	uint32_t NumberOfFiles;
	uint32_t count;
	uint32_t fileId;
	char BasePath[32];
public:
	file_header()
	{
		loadPtrToTigerFile(this);
	}
	void printHeader()
	{
		cout << "Tiger Header\n";
		cout << "Magic\t\t:" << hex << magic << dec;
		cout << "\nVersion\t\t:" << version;
		cout << "\nParts\t\t:" << NumberOfFiles;
		cout << "\nDRMs\t\t:" << count;
		cout << "\nBase Path\t:" << BasePath;
		cout << endl;
	}
	element* getElements()
	{
		return (element*)((char*)this + sizeof(file_header));
	}
	uint32_t getElementCount()
	{
		return count;
	}
	uint32_t getFileCount() { return NumberOfFiles; }
	void loadPtrToTigerFile(void* ptr)
	{
		uint32_t count = (uint32_t)count_if(tigerPtrMap.begin(), tigerPtrMap.end(), [&](auto base)->bool {return ((base.first&(fileId << 4)) == (fileId << 4)); });
		tigerPtrMap[(fileId << 4) | (count & 0x0F)] = ptr;
	}
	void load(iostream *dataStream)
	{
		dataStream->read((char*)this, sizeof(file_header));
	}
	void save(iostream *dataStream)
	{
		dataStream->write((char*)this, sizeof(file_header));
	}
	vector<element> it(fstream* filestream)
	{
		vector<struct element> v(count);
		filestream->read((char*)v.data(), v.size() * sizeof(element));
		return v;
	}
};
// TIGER END

#pragma pack(push,1)
struct PCD9_Header
{
	char magic[4];
	union
	{
		uint32_t format;
		char formatName[4];
	};
	uint32_t size;
	uint32_t u1;
	uint16_t width, height;
	uint8_t bpp;
	uint16_t u2;
	uint8_t mipmap;
	uint32_t ddsFlags;
	char* getDataPtr()
	{
		return (char*)(&this[1]);
	}
};
#pragma pack(pop)
#endif // !TIGER_H

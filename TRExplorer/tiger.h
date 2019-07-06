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
#include <string>
#include <fstream>
#include "cdrm.h"

#define ALIGN_TO(address,alignment) (((address+alignment-1)/alignment)*alignment)
#define TIGER_FILEID_MASK_V1 (0x7FF)
#define TIGER_FILEID_MASK_V2 (0xF)
#define NAME_LENGTH		(256)

#define TIGER_MAGIC		(0x53464154)
//	"NEXT"
#define CDRM_FOOTER_MAGIC		(0x5458454E)

using namespace std;

extern map<uint32_t, void*> tigerPtrMap;
extern map<uint32_t, string> fileListHashMap;

struct unknown_header1
{
	uint32_t DataSize;
	uint32_t ddsFlags;
	uint32_t flags;
	uint32_t ID;
	uint32_t u3;
};

class unknown_header2_v1
{
public:
	uint32_t u1;
	uint32_t offset;
	uint32_t size;
	uint32_t u4;
	int setSize(size_t newSize);
	CDRM_Header* getCDRMPtr()
	{
		if (tigerPtrMap.find(offset & TIGER_FILEID_MASK_V1) != tigerPtrMap.end())
		{
			return (CDRM_Header*)((char*)(tigerPtrMap[offset & TIGER_FILEID_MASK_V1]) + (offset&(~TIGER_FILEID_MASK_V1)));
		}
		else
		{
			fprintf(stderr, "Error occured trying to read offset [%#X]. Reading other files not implimented\n", offset);
			return nullptr;
		}
	}
	uint32_t getCDRMDataSize()
	{
		return size;
	}
	CDRM_BlockFooter* getCDRMFooter();
};

class unknown_header2_v2
{
public:
	uint32_t u1;
	uint32_t u2;
	uint32_t flags;
	uint32_t offset;
	uint32_t size;
	uint32_t u3;
	int setSize(size_t newSize);
	CDRM_Header* getCDRMPtr();
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
	DRM_Header(){}
	~DRM_Header(){}
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
	void* getUnknownHeaders2()
	{
		char* tmp = (char*)this + sizeof(DRM_Header) + ((SectionCount * sizeof(unknown_header1)) + strlen1 + strlen2);
		return tmp;
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
class element_v1
{
public:
	uint32_t nameHash;
	uint32_t Local;
	uint32_t size;
	uint32_t offset;
	void* getDataPtr()
	{
		if (tigerPtrMap.find(offset & TIGER_FILEID_MASK_V1) != tigerPtrMap.end())
		{
			return (void*)((char*)(tigerPtrMap[offset & TIGER_FILEID_MASK_V1]) + (offset&(~TIGER_FILEID_MASK_V1)));
		}
		else
		{
			printf("Error occured trying to read offset [%#X]", offset);
			exit(1);
            return nullptr;
		}
	}
	uint32_t getNameHash()
	{
		return nameHash;
	}
};

class element_v2
{
public:
	uint32_t nameHash;
	uint32_t Local;
	uint32_t size;
	uint32_t unknown;
	uint32_t flags;
	uint32_t offset;
	void* getDataPtr()
	{
		if (tigerPtrMap.find(offset & TIGER_FILEID_MASK_V2) != tigerPtrMap.end())
		{
			return (void*)((char*)(tigerPtrMap[offset & TIGER_FILEID_MASK_V2]) + (offset&(~TIGER_FILEID_MASK_V2)));
		}
		else
		{
			printf("Error occured trying to read offset [%#X]", offset);
			exit(1);
            return nullptr;
		}
	}
	uint32_t getNameHash()
	{
		return nameHash;
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

		printf("Tiger Header, Game version [%d], files [%d]\n", version, NumberOfFiles);
	}
	void* getElement(int index)
	{
		char* tmp = (char*)this + sizeof(file_header);
		if (version == 0x04)
		{
			return  (element_v2*)tmp + index;
		}
		else
		{
			return  (element_v1*)tmp + index;
		}
	}
	uint32_t getElementCount()
	{
		return count;
	}
	uint32_t getFileCount() { return NumberOfFiles; }
	void loadPtrToTigerFile(void* ptr)
	{
		uint32_t count = (uint32_t)count_if(tigerPtrMap.begin(), tigerPtrMap.end(), [&](auto base)->bool {return (version == 4) ? true : ((base.first&(fileId << 4)) == (fileId << 4)); });
		if (version == 0x04)
		{
			tigerPtrMap[count & 0x0F] = ptr;
		}
		else
		{
			tigerPtrMap[(fileId << 4) | (count & 0x0F)] = ptr;
		}
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

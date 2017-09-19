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
#include "alldef.h"

map<uint32_t, void*> tigerPtrMap;
map<uint32_t, string> fileListHashMap;

int unknown_header2_v1::setSize(size_t newSize)
{
	CDRM_BlockFooter *footer = getCDRMFooter();
	CDRM_Header* nextCdrm = reinterpret_cast<CDRM_Header*>((char*)footer + footer->relative_offset);
	size = (uint32_t)newSize;
	CDRM_BlockFooter *newfooter = getCDRMFooter();
	((uint32_t*)newfooter->magic)[0] = 'TXEN';
	newfooter->relative_offset = (uint32_t)((char*)nextCdrm - (char*)newfooter);
	return 0;
}

CDRM_BlockFooter * unknown_header2_v1::getCDRMFooter()
{
	if (tigerPtrMap.find(offset & TIGER_FILEID_MASK_V1) != tigerPtrMap.end())
	{
		return (CDRM_BlockFooter*)((char*)(tigerPtrMap[offset & TIGER_FILEID_MASK_V1]) + (offset&(~TIGER_FILEID_MASK_V1)) + (((size + 0xF) / 0x10) * 0x10));
	}
	else
	{
		printf("Error occured trying to read offset [%#X]", offset);
		exit(1);
		return NULL;
	}
}

CDRM_Header * unknown_header2_v2::getCDRMPtr()
{
	if (tigerPtrMap.find(flags & TIGER_FILEID_MASK_V2) != tigerPtrMap.end())
	{
		return (CDRM_Header*)((char*)(tigerPtrMap[flags & TIGER_FILEID_MASK_V2]) + (offset&(~TIGER_FILEID_MASK_V2)));
	}
	else
	{
		fprintf(stderr, "Error occured trying to read offset [%#X]. Not implimented\n", offset);
		return nullptr;
	}
}

CDRM_BlockFooter * unknown_header2_v2::getCDRMFooter()
{
	if (tigerPtrMap.find(offset & TIGER_FILEID_MASK_V2) != tigerPtrMap.end())
	{
		return (CDRM_BlockFooter*)((char*)(tigerPtrMap[offset & TIGER_FILEID_MASK_V2]) + (offset&(~TIGER_FILEID_MASK_V2)) + (((size + 0xF) / 0x10) * 0x10));
	}
	else
	{
		printf("Error occured trying to read offset [%#X]", offset);
		exit(1);
		return NULL;
	}
}

uint32_t * StringTable_Header::getStringIndicesPtr()
{
	return (uint32_t*)((char*)(&this[1]) + (unknownCount * sizeof(uint32_t)));
}

uint32_t StringTable_Header::getStringIndicesCount()
{
	return tableRange;
}

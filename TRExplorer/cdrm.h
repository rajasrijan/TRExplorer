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
#pragma once
#include <stdint.h>
#include <string>
#include <fstream>

using namespace std;

#define CDRM_MAX_BLOCK_SIZE (0x40000ull)

#define CDRM_BLOCK_FOOTER_MAGIC (0x5458454E)

struct CDRM_BlockFooter //(repeaded "count" times , 16 byte aligned at the end)
{
	union
	{
		uint8_t magic[4]; // Always "NEXT"
		uint32_t uiMagic;
	};
	uint32_t relative_offset;
};

#pragma pack(push,1)
class CDRM_DataChunks //(repeaded "count" times , 16 byte aligned at the end)
{
public:
	uint32_t blockType : 8; // 1 is uncompressed , 2 is zlib , 3 is the new one
	uint32_t uncompressedSize : 24; //size of uncompressed data chunk , max 0x40000
	uint32_t compressedSize; //size of the compressed data block
	CDRM_DataChunks();
	~CDRM_DataChunks();
};
#pragma pack(pop)

class CDRM_Header //CDRM Header file.
{
public:
	uint32_t magic; // Always "CDRM" , so 0x4344524d in BE , 0x4d524443 in LE
	uint32_t version; // Always 0 for TR9? Was 0x2 for some files in DX3 I think.
	uint32_t count; // number of blocks
	uint32_t offset; // zero.pretty much everything is 16 byte aligned
	void printHeader();

	int getData(char *dstPtr, size_t size);
	int setData(const char *srcPtr, size_t size, size_t& totalSize, size_t availableSz);
	size_t getDataSize();
	size_t getCompressedSize();
};

static inline void dumpData(string fn, char* pData, size_t sz)
{
	fstream f(fn, ios_base::binary | ios_base::out);
	f.write(pData, sz);
}
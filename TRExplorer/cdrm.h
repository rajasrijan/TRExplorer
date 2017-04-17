#pragma once
#include <stdint.h>

struct CDRM_BlockFooter //(repeaded "count" times , 16 byte aligned at the end)
{
	uint8_t magic[4]; // Always "NEXT"
	uint32_t relative_offset;
};

class CDRM_DataChunks //(repeaded "count" times , 16 byte aligned at the end)
{
public:
	uint32_t blockType : 8; // 1 is uncompressed , 2 is zlib , 3 is the new one
	uint32_t uncompressedSize : 24; //size of uncompressed data chunk , max 0x40000
	uint32_t compressedSize; //size of the compressed data block
	CDRM_DataChunks();
	~CDRM_DataChunks();
};

class CDRM_Header //CDRM Header file.
{
public:
	uint32_t magic; // Always "CDRM" , so 0x4344524d in BE , 0x4d524443 in LE
	uint32_t version; // Always 0 for TR9? Was 0x2 for some files in DX3 I think.
	uint32_t count; // number of blocks
	uint32_t offset; // zero.pretty much everything is 16 byte aligned
	void printHeader();
	
	int getData(char *dstPtr, size_t size);
	int setData(char *dstPtr, size_t size, size_t& totalSize);
	size_t getDataSize();
	size_t getCompressedSize();
};


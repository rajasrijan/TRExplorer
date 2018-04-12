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
#include "cdrm.h"
#include <fstream>
#include <iostream>
#include <zlib.h>
#include <vector>
#include <memory>
#include <algorithm>
#include <string>
#include <errno.h>
#include <cstring>

#define ALIGN_TO(address,alignment) (((address+alignment-1)/alignment)*alignment)
using namespace std;

void CDRM_Header::printHeader()
{
	cout << "CDRM Header\n";
	cout << "Magic\t\t:" << hex << magic << dec;
	cout << "\nVersion\t\t:" << version;
	cout << "\nDRMs\t\t:" << count;
	cout << "\noffset\t:" << offset;
	cout << endl;
}

int CDRM_Header::getData(char* dstPtr, size_t size)
{
	if (magic != 0x4d524443)
	{
		printf("Error: CDRM magic didnt match\n");
	}
	int errorCode = 0;
	if (size < getDataSize())
	{
		return 1;
	}
	CDRM_DataChunks* chunks = (CDRM_DataChunks*) &this[1];
	char* dataPtr = (char*) &this[1] + ((((sizeof(CDRM_DataChunks) * count) + 15) / 16) * 16);
	for (size_t i = 0; i < count; i++)
	{
		switch (chunks[i].blockType)
		{
		case 1:	//uncompressed
		{
			memcpy(dstPtr, dataPtr, chunks[i].uncompressedSize);
			dataPtr += ALIGN_TO(chunks[i].uncompressedSize, 16);
			dstPtr += chunks[i].uncompressedSize;
		}
			break;
		case 2:	//zlib
		{
			uLongf bytesWritten = chunks[i].uncompressedSize;
			if ((errorCode = uncompress((Bytef*) dstPtr, &bytesWritten, (Bytef*) dataPtr, chunks[i].compressedSize)) != Z_OK)
			{
				cout << "\nError: Uncompress failure. Error code:" << errorCode;
				fstream dump("compressed_data_dump_" + to_string(chunks[i].compressedSize) + "_" + to_string(chunks[i].uncompressedSize),
						ios_base::binary | ios_base::out);
				dump.write(dataPtr, chunks[i].compressedSize);
				dump.close();
				exit(1);
			}
			dataPtr += ALIGN_TO(chunks[i].compressedSize, 16);
			dstPtr += chunks[i].uncompressedSize;
		}
			break;
		default:
			cout << "\nError: Unknown block type.";
			exit(1);
		}
	}
	return 0;
}

int CDRM_Header::setData(const char * srcPtr, size_t size, size_t& totalSize, size_t availableSz)
{
	int errorCode = 0;
	totalSize = 0;
	size_t requiredSize = 0;
	CDRM_DataChunks* chunks = (CDRM_DataChunks*) &this[1];
	int blockType = chunks[0].blockType;
	//	save for safe calculations
	vector<CDRM_DataChunks> tmpChunk;
	vector<shared_ptr<char>> dataChunk;

	//	pack stuff
	for (long long unsigned int bytesProcessed = 0; bytesProcessed < size; bytesProcessed += CDRM_MAX_BLOCK_SIZE)
	{
		CDRM_DataChunks chunkDescriptor;
		chunkDescriptor.blockType = blockType;
		chunkDescriptor.uncompressedSize = min(CDRM_MAX_BLOCK_SIZE, max(size - bytesProcessed, 0ull));
		printf("chunkDescriptor.uncompressedSize [%#X]\n", chunkDescriptor.uncompressedSize);
		if (chunkDescriptor.blockType == 1)
		{
			return 1;
		}
		else if (chunkDescriptor.blockType == 2)
		{
			char *dstData = new char[chunkDescriptor.uncompressedSize];
			uLong bytesWritten = chunkDescriptor.uncompressedSize;
			if ((errorCode = compress2((Bytef*) dstData, &bytesWritten, (Bytef*) &srcPtr[bytesProcessed], chunkDescriptor.uncompressedSize, 9)) != Z_OK)
			{
				cout << "\nError: Compress failure. Error code:" << errorCode;
				return 1;
			}
			chunkDescriptor.compressedSize = bytesWritten;
			dataChunk.push_back(shared_ptr<char>(dstData));
		}
		requiredSize += chunkDescriptor.compressedSize;
		tmpChunk.push_back(chunkDescriptor);
	}
	//dumpData("compressed_data_dump", dataChunk.back().get(), tmpChunk.back().compressedSize);
	if (requiredSize > availableSz)
	{
		printf("Not enough space to store imported data.\n");
		return 1;
	}
	//	set the new chunk count
	count = (uint32_t) tmpChunk.size();
	//	writeout chunk headers
	memcpy(chunks, tmpChunk.data(), count * sizeof(CDRM_DataChunks));
	//	writeout packed data
	char* packedDataPtr = (char*) chunks + (count * sizeof(CDRM_DataChunks));
	for (size_t i = 0; i < dataChunk.size(); i++)
	{
		//	align data
		packedDataPtr = (char*) ALIGN_TO((uint64_t )packedDataPtr, 16);
		memcpy(packedDataPtr, dataChunk[i].get(), tmpChunk[i].compressedSize);
		packedDataPtr += tmpChunk[i].compressedSize;
	}
	totalSize = (uint64_t) packedDataPtr - (uint64_t) chunks;
	return 0;
}

size_t CDRM_Header::getDataSize()
{
	size_t total_size = 0;
	CDRM_DataChunks* chunks = (CDRM_DataChunks*) &this[1];
	for (size_t i = 0; i < count; i++)
	{
		total_size += chunks[i].uncompressedSize;
	}
	return total_size;
}

size_t CDRM_Header::getCompressedSize()
{
	size_t total_size = 0;
	CDRM_DataChunks* chunks = (CDRM_DataChunks*) &this[1];
	for (size_t i = 0; i < count; i++)
	{
		total_size += chunks[i].compressedSize;
	}
	return total_size;
}

CDRM_DataChunks::CDRM_DataChunks() :
		blockType(0), uncompressedSize(0), compressedSize(0)
{
}

CDRM_DataChunks::~CDRM_DataChunks()
{
}

#include "cdrm.h"
#include <iostream>
#include <zlib.h>

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
	int errorCode = 0;
	if (size < getDataSize())
	{
		return 1;
	}
	CDRM_DataChunks* chunks = (CDRM_DataChunks*)&this[1];
	char* dataPtr = (char*)&this[1] + ((((sizeof(CDRM_DataChunks)*count) + 15) / 16) * 16);
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
			if ((errorCode = uncompress((Bytef*)dstPtr, &bytesWritten, (Bytef*)dataPtr, chunks[i].compressedSize)) != Z_OK)
			{
				cout << "\nError: Uncompress failure. Error code:" << errorCode;
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

int CDRM_Header::setData(char * srcPtr, size_t size, size_t& totalSize)
{
	int errorCode = 0;
	totalSize = 0;
	size_t availableSize = getCompressedSize();
	if (size != getDataSize())
	{
		return 1;
	}
	CDRM_DataChunks* chunks = (CDRM_DataChunks*)&this[1];
	char* dataPtr = (char*)&this[1] + ((((sizeof(CDRM_DataChunks)*count) + 15) / 16) * 16);
	char* dataStartPtr = dataPtr;
	for (size_t i = 0; i < count; i++)
	{
		switch (chunks[i].blockType)
		{
		case 1:	//uncompressed
		{
			memcpy(dataPtr, srcPtr, chunks[i].uncompressedSize);
			dataPtr += ALIGN_TO(chunks[i].uncompressedSize, 16);
			srcPtr += chunks[i].uncompressedSize;
			size -= chunks[i].uncompressedSize;
		}
		break;
		case 2:	//zlib
		{
			uLongf bytesWritten = (uLongf)availableSize;
			if ((errorCode = compress2((Bytef*)dataPtr, &bytesWritten, (Bytef*)srcPtr, chunks[i].uncompressedSize, 9)) != Z_OK)
			{
				cout << "\nError: Uncompress failure. Error code:" << errorCode;
				exit(1);
			}
			chunks[i].compressedSize = bytesWritten;
			availableSize -= bytesWritten;
			dataPtr += ALIGN_TO(chunks[i].compressedSize, 16);
			srcPtr += chunks[i].uncompressedSize;
			size -= chunks[i].uncompressedSize;
		}
		break;
		default:
			cout << "\nError: Unknown block type.";
			exit(1);
		}
	}
	totalSize = dataPtr - dataStartPtr;
	return 0;
}

size_t CDRM_Header::getDataSize()
{
	size_t total_size = 0;
	CDRM_DataChunks* chunks = (CDRM_DataChunks*)&this[1];
	for (size_t i = 0; i < count; i++)
	{
		total_size += chunks[i].uncompressedSize;
	}
	return total_size;
}

size_t CDRM_Header::getCompressedSize()
{
	size_t total_size = 0;
	CDRM_DataChunks* chunks = (CDRM_DataChunks*)&this[1];
	for (size_t i = 0; i < count; i++)
	{
		total_size += chunks[i].compressedSize;
	}
	return total_size;
}

CDRM_DataChunks::CDRM_DataChunks()
{
}

CDRM_DataChunks::~CDRM_DataChunks()
{
}

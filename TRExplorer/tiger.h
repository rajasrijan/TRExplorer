#ifndef TIGER_H
#define TIGER_H

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <stdint.h>
#include <memory>
#include <map>

using namespace std;

//	DRM start.
struct DRM_Header
{
	uint32_t version; // 0x16 for TR9
	uint32_t strlen1, strlen2;
	uint32_t unknown[3]; // all zero? must check all DRM files
	uint32_t SectionCount;
	uint32_t UnknownCount;
};

struct unknown_header1
{
	uint32_t DataSize;
	uint32_t type;
	uint32_t flags;
	uint32_t ID;
	uint32_t u3;
};
struct unknown_header2
{
	uint32_t u1;
	uint32_t offset;
	uint32_t size;
	uint32_t u4;
};

//DRM end

// TIGER START
struct file_header
{
	uint32_t magic;
	uint32_t version;
	uint32_t NumberOfFiles;
	uint32_t count, d4;
	char BasePath[32];
};

struct element
{
	uint32_t nameHash;
	uint32_t Local;
	uint32_t size;
	uint32_t offset;
};

// TIGER END


class CDRM_Header //CDRM Header file.
{
public:
	uint32_t magic; // Always "CDRM" , so 0x4344524d in BE , 0x4d524443 in LE
	uint32_t version; // Always 0 for TR9? Was 0x2 for some files in DX3 I think.
	uint32_t count; // number of blocks
	uint32_t u1; // pretty much everything is 16 byte aligned
	CDRM_Header()
	{}
	~CDRM_Header()
	{}
	void load(iostream *dataStream)
	{
		dataStream->read((char*)this, sizeof(CDRM_Header));
	}

};

class CDRM_BlockHeader //(repeaded "count" times , 16 byte aligned at the end)
{
public:
	uint32_t blockType : 8; // 1 is uncompressed , 2 is zlib , 3 is the new one
	uint32_t uncompressedSize : 24; //size of uncompressed data chunk , max 0x40000
	uint32_t compressedSize; //size of the compressed data block
	CDRM_BlockHeader()
	{}
	~CDRM_BlockHeader()
	{}
	void load(iostream *dataStream)
	{
		dataStream->read((char*)this, sizeof(CDRM_BlockHeader));
	}
};

struct CDRM_BlockFooter //(repeaded "count" times , 16 byte aligned at the end)
{
	uint8_t magic[4]; // Always "NEXT"
	uint32_t relative_offset;
};

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
	uint8_t mipmap;
	uint16_t u2;
	uint32_t type;
};

class TIGER
{
	enum DataType
	{
		DRMT = 0x00000016,
		CDRM = 0x4D524443,
		PCD9 = 0x50434439,
		UNKNOWN1 = 0x32383133	//	Appears to be text
	};
	enum Mode
	{
		PACK,
		UNPACK
	};
private:
	vector<fstream*> tigerFiles;
	file_header header;
	string basePath;
	Mode currentMode;
public:
	map<uint32_t, uint32_t> header_type_counter;
	bool writeDRM;
	bool writeDDS;
	vector<element> elements;
	TIGER(string fileLocation, bool _writeDRM = false);
	~TIGER(void);
	fstream* getRepositionedStream(uint32_t offset);
	void printHeader();
	auto_ptr<char> getData(int id, uint32_t &size);
	void getDate(int id, char* buf, uint32_t size);
	uint32_t getEntryCount();
	DataType getType(iostream *dataStream);
	auto_ptr<char> getDecodedData(int id, uint32_t &size, string path = "tmp");
	bool decodeDRM(iostream *dataStream, uint32_t &size, string &path, string &name);
	uint32_t decodeCDRM(iostream *dataStream, uint32_t &size, string &path, string &name);
	auto_ptr<char> decodePCD9(auto_ptr<char> dataStream, uint32_t &size, string &path, string &name);
	void dumpRAW(iostream *dataStream, uint32_t &size, string &path, string &name);
	bool pack(uint32_t nameHash, string path);
	bool unpack(uint32_t id, string path);

	void listAll()
	{
		for (int i = 0; i < 200; i++)
		{
			printf_s("ID=%d,Hashname: %08x\n", i, elements[i].nameHash);
		}
	}

	int findByHash(uint32_t nameHash)
	{
		for (uint32_t i = 0; i < elements.size(); i++)
		{
			if (nameHash == elements[i].nameHash)
			{
				return i;
			}
		}
		return -1;
	}

	void scanCDRM()
	{

	}
};


#endif // !TIGER_H

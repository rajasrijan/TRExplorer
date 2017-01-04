#ifndef TIGER_H
#define TIGER_H

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <stdint.h>
#include <memory>
#include <map>

#define ALIGN_TO(address,alignment) ((address+alignment-1)/alignment)*alignment

using namespace std;

//	DRM start.

struct unknown_header1
{
	uint32_t DataSize;
	uint32_t type;
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
		dataStream->read((char*)vec.data(), SectionCount*sizeof(unknown_header2));
		return vec;
	}
};

//DRM end

// TIGER START
struct element
{
	uint32_t nameHash;
	uint32_t Local;
	uint32_t size;
	uint32_t offset;
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
		filestream->read((char*)v.data(), v.size()*sizeof(element));
		return v;
	}
};
// TIGER END

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

	void save(iostream *dataStream)
	{
		dataStream->write((char*)this, sizeof(CDRM_BlockHeader));
	}
};

class CDRM_Header //CDRM Header file.
{
public:
	uint32_t magic; // Always "CDRM" , so 0x4344524d in BE , 0x4d524443 in LE
	uint32_t version; // Always 0 for TR9? Was 0x2 for some files in DX3 I think.
	uint32_t count; // number of blocks
	uint32_t offset; // zero.pretty much everything is 16 byte aligned
	CDRM_Header()
	{}
	~CDRM_Header()
	{}
	void load(iostream *dataStream)
	{
		dataStream->read((char*)this, sizeof(CDRM_Header));
	}
	void printHeader()
	{
		cout << "CDRM Header\n";
		cout << "Magic\t\t:" << hex << magic << dec;
		cout << "\nVersion\t\t:" << version;
		cout << "\nDRMs\t\t:" << count;
		cout << "\noffset\t:" << offset;
		cout << endl;
	}
	vector<CDRM_BlockHeader> it(iostream *dataStream)
	{
		vector<CDRM_BlockHeader> vec(count);
		dataStream->read((char*)vec.data(), count*sizeof(CDRM_BlockHeader));
		return vec;
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

template<class T>
class FormatHelperNoChild
{
private:
	T t;
	int offset;
	fstream *filestream;
public:
	FormatHelperNoChild()
	{
		offset = 0;
		filestream = 0;
	}
	FormatHelperNoChild(int off, fstream *file) :offset(off), filestream(file)
	{
		data->seekg(offset);
		t.load(filestream);
	}

	FormatHelperNoChild(int off, fstream *file, T &_t) :offset(off), filestream(file), t(_t)
	{

	}

	~FormatHelperNoChild()
	{

	}

	T& get()
	{
		return t;
	}

	void setOffset(int off)
	{
		offset = off;
	}
	void setFile(fstream* stream)
	{
		filestream = stream;
	}

	void serialize()
	{
		filestream->seekp(offset);
		filestream->write((char*)&t, sizeof(T));
	}

	static size_t nativeSize()
	{
		return sizeof(T);
	}
};

template<class T, class sT>
class FormatHelper
{
private:
	T t;
	int offset;
	fstream *filestream;
	//sT vec;
public:
	FormatHelper() :offset(0), filestream(0)
	{
		memset((void*)&t, 0, sizeof(T));
	}
	FormatHelper(int off, fstream *file) :offset(off), filestream(file)
	{
		filestream->seekg(offset);
		t.load(filestream);
	}

	FormatHelper(int off, fstream *file, T &_t) :offset(off), filestream(file), t(_t)
	{

	}

	~FormatHelper()
	{

	}

	T& get()
	{
		return t;
	}

	static size_t nativeSize()
	{
		return sizeof(T);
	}

	vector<sT> getIterator(int padd = 0)
	{
		int child_offset = offset + sizeof(T) + padd;
		filestream->seekg(child_offset);
		auto tmp = t.it(filestream);
		vector<sT> vec;

		for (auto it = tmp.begin(); it < tmp.end(); it++)
		{
			vec.push_back(sT(child_offset, filestream, *it));
			child_offset += sT::nativeSize();
		}
		return vec;
	}

	void save()
	{
		filestream->seekp(offset);
		t.save(filestream);
	}
};

typedef FormatHelperNoChild<CDRM_BlockHeader>	CDRMBlockHeader;
typedef FormatHelper<CDRM_Header, CDRMBlockHeader> CDRMHeader;
typedef FormatHelperNoChild<unknown_header1> unknownHeader1;
typedef FormatHelper<unknown_header2, CDRMHeader> unknownHeader2;
typedef FormatHelper<DRM_Header, unknownHeader2> DRMHeader;
typedef FormatHelper<element, DRMHeader> elementHeader;
typedef FormatHelper<file_header, elementHeader> fileHeader;

#endif // !TIGER_H

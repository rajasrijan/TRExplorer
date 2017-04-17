#define _CRT_SECURE_NO_WARNINGS
#include "patch.h"
#include <algorithm>
#include <regex>
#include <assert.h>
#include "alldef.h"
#include "zlib.h"
#include "crc32.h"
#include <direct.h>


bool isDir(string pathname)
{
	struct stat info;

	if (stat(pathname.c_str(), &info) != 0)
		return false;
	else if (info.st_mode & S_IFDIR)  // S_ISDIR() doesn't exist on my windows 
		return true;
	else
		return false;
}

int makeDirectory(string path, bool recursive = false)
{
	if (!recursive && !isDir(path))
		return _mkdir(path.c_str());
	else
	{
		return 0;
	}
	return 0;
}

bool isFileExists(const std::string& file) {
	struct stat buf;
	return (stat(file.c_str(), &buf) == 0);
}

bool dir(vector<string> &resultList, string pattern = "")
{
	string command = "dir /B \"" + pattern + "\"";
	FILE* dirStdout = _popen(command.c_str(), "r");
	char buffer[1024];
	char* line = nullptr;
	while ((line = fgets(buffer, sizeof(buffer), dirStdout)) != nullptr)
	{
		size_t pathLength = strlen(line);
		if (pathLength > 0 && line[pathLength - 1] == '\n')
			line[pathLength - 1] = 0;
		resultList.push_back(line);
	}
	_pclose(dirStdout);
	return 0;
}

string pathJoin(string first, string second)
{
	bool fe = (*first.rbegin()) == '\\';
	bool ss = (*second.begin()) == '\\';
	if (fe == false && ss == false)
	{
		return first + "\\" + second;
	}
	else if ((fe == true && ss == false) || (fe == false && ss == true))
	{
		return first + second;
	}
	else if ((fe == true && ss == true))
	{
		return first + second.substr(1, second.length() - 1);
	}
	return "";
}

std::streampos getFileSize(fstream *file)
{
	std::streampos pos = file->tellg();
	file->seekg(0, SEEK_END);
	std::streampos endPos = file->tellg();
	file->seekg(pos, SEEK_SET);
	return endPos;
}

//int getXmlNodeAttribute(const Node* pNode, const string &attrName, uint32_t &attrVal)
//{
//	if (!pNode)
//		return 1;
//	AutoPtr<NamedNodeMap> attributes = pNode->attributes();
//	if (attributes.isNull())
//		return 1;
//	Node *attrNode = attributes->getNamedItem(attrName);
//	if (!attrNode)
//		return 1;
//	attrVal = atoi(attrNode->getNodeValue().c_str());
//	return 0;
//}

//int getXmlNodeValue(const Node* pNode, string &val)
//{
//	if (!pNode)
//		return 1;
//	val = pNode->getNodeValue();
//	return 0;
//}

patch::patch(string _tigerPath, bool silent) :tigerFilePath(_tigerPath), tiger(NULL)
{
	HANDLE mainTigerFile = CreateFile(tigerFilePath.c_str(), GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (mainTigerFile == INVALID_HANDLE_VALUE)
	{
		printf("Not able to open file [%s]. error(%d)", tigerFilePath.c_str(), GetLastError());
		exit(1);
	}
	fileHandles.push_back(mainTigerFile);
	HANDLE tigetFileMapping = CreateFileMapping(mainTigerFile, NULL, PAGE_READWRITE, 0, 0, NULL);
	if (tigetFileMapping == INVALID_HANDLE_VALUE)
	{
		printf("Not able to create file mapping [%s]. error(%d)", tigerFilePath.c_str(), GetLastError());
		exit(1);
	}
	fileHandles.push_back(tigetFileMapping);
	void* tigerFilePtr = MapViewOfFile(tigetFileMapping, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
	if (tigerFilePtr == nullptr)
	{
		printf("Not able to memory map file [%s]. error(%d)", tigerFilePath.c_str(), GetLastError());
		exit(1);
	}
	tigerPtrList.push_back(tigerFilePtr);
	tiger = new(tigerFilePtr) file_header();
	string basePath = tigerFilePath.substr(0, tigerFilePath.find(".000.tiger"));
	for (uint32_t i = 1; i < tiger->getFileCount(); i++)
	{
		char fileNo[32] = { 0 };
		sprintf(fileNo, ".%03d.tiger", i);
		tigerFilePath = basePath + fileNo;
		HANDLE tigerFile = CreateFile(tigerFilePath.c_str(), GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (tigerFile == INVALID_HANDLE_VALUE)
		{
			printf("Not able to open file [%s]. error(%d)", tigerFilePath.c_str(), GetLastError());
			exit(1);
		}
		fileHandles.push_back(tigerFile);
		HANDLE tigetFileMapping = CreateFileMapping(tigerFile, NULL, PAGE_READWRITE, 0, 0, NULL);
		if (tigetFileMapping == INVALID_HANDLE_VALUE)
		{
			printf("Not able to create file mapping [%s]. error(%d)", tigerFilePath.c_str(), GetLastError());
			exit(1);
		}
		fileHandles.push_back(tigetFileMapping);
		void* tigerFilePtr = MapViewOfFile(tigetFileMapping, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
		if (tigerFilePtr == nullptr)
		{
			printf("Not able to memory map file [%s]. error(%d)", tigerFilePath.c_str(), GetLastError());
			exit(1);
		}
		tigerPtrList.push_back(tigerFilePtr);
		tiger->loadPtrToTigerFile(tigerFilePtr);
	}
	ifstream filelist("filelist.txt");
	if (filelist.is_open())
	{
		char buffer[4096] = { 0 };
		string basePath = tiger->BasePath;
		do
		{
			filelist >> buffer;
			string fullFilePath = basePath + buffer;
			uint32_t crc = crc32_hash(fullFilePath.data(), fullFilePath.length());
			fileListHashMap[crc] = fullFilePath;
		} while (!filelist.eof());
		filelist.close();
	}
}

patch::~patch()
{
	//	Destroy in reverse order.
	for (auto fileMapPtrIt = tigerPtrList.rbegin(); fileMapPtrIt != tigerPtrList.rend(); fileMapPtrIt++)
	{
		UnmapViewOfFile(*fileMapPtrIt);
	}
	//	Destroy in reverse order.
	for (auto handleIt = fileHandles.rbegin(); handleIt != fileHandles.rend(); handleIt++)
	{
		CloseHandle(*handleIt);
	}
}

int patch::unpackAll(string path)
{
	int ret = 0;
	for (uint32_t i = 0; i < tiger->count && !ret; i++)
	{
		ret = unpack(i, path, true);
	}
	return 0;
}

//	cdrmId==-1 means all
int patch::process(int id, int cdrmId, const string& path, bool isPacking, bool silent)
{
	//load table
	int errorCode = 0;
	uint32_t hdr = 0;
	element* elements = tiger->getElements();

	if (id<0 || id >(int)tiger->getElementCount())
		return 1;
	element &element = elements[id];
	if (!isPacking)
		system(("mkdir " + path).c_str());
	char* dataPtr = (char*)element.getDataPtr();
	hdr = ((uint32_t*)dataPtr)[0];
	switch (hdr)
	{
	case 0:
	case 1:
	case 2:
	{
		cout << "LOCAL" << endl;
		string subDir = path + "\\" + to_string(id);
		string localFilePath = path + "\\" + to_string(id) + ".local";
		{
			fstream localDump(localFilePath, ios_base::binary | ios_base::out);
			localDump.write(dataPtr, element.size);
		}
		fstream textFileObject(localFilePath + ".txt", isPacking ? ios_base::in : ios_base::out);
		StringTable_Header* stringTable = reinterpret_cast<StringTable_Header*>(dataPtr);
		if (!isPacking)
		{
			textFileObject << stringTable->local << " " << stringTable->unknownCount << " " << stringTable->tableRange << endl;
			uint32_t *strIndexPtr = stringTable->getStringIndicesPtr();
			for (size_t index = 0; index < stringTable->getStringIndicesCount(); index++)
			{
				if (strIndexPtr[index])
				{
					for (uint8_t* str = reinterpret_cast<uint8_t*>(&dataPtr[strIndexPtr[index]]); *str != 0; str++)
					{
						if (iscntrl(*str) || *str == '&')
						{
							textFileObject << "&" << (int)(*str);
						}
						else
						{
							textFileObject << *str;
						}
					}
				}
				textFileObject << endl;
			}
		}
		else
		{
			textFileObject >> stringTable->local >> stringTable->unknownCount >> stringTable->tableRange;
		}
	}
	break;
	case 0x16:
	{
		cout << "DRM" << endl;
		DRM_Header* drmHeader = new(dataPtr) DRM_Header;
		unknown_header2 *uh2 = drmHeader->getUnknownHeaders2();
		string subDir = path + "\\" + to_string(id);
		if (!errorCode && !isPacking)
			system(("mkdir " + subDir).c_str());

		for (size_t uh2_section = ((cdrmId == -1) ? 0 : cdrmId);
			uh2_section < ((cdrmId == -1) ? drmHeader->getUnknownHeaders2Count() : cdrmId + 1);
			uh2_section++)
		{
			string subDir = path + "\\" + to_string(id);
			string cdrmFilePath = subDir + "\\" + to_string(uh2_section) + ".cdrm";
			CDRM_Header* cdrmHeader = uh2[uh2_section].getCDRMPtr();
			if (!cdrmHeader)
				continue;

			int cdrmFileFlag = 0;
			if (isPacking)
				cdrmFileFlag = ios_base::binary | ios_base::in;
			else
				cdrmFileFlag = ios_base::binary | ios_base::out;

			/*fstream cdrmFile(cdrmFilePath, cdrmFileFlag);
			size_t cdrmFileSize = 0;
			if (cdrmFile.is_open())
			{
				if (!isPacking)
				{
					cdrmFile.write((char*)cdrmHeader, uh2[uh2_section].getCDRMDataSize());
				}
				else
				{
					cdrmFileSize = getFileSize(&cdrmFile);
				}
			}
			cdrmFile.close();*/
			string dmpFilePath = subDir + "\\" + to_string(uh2_section) + ".dmp";
			/*if (isPacking && !isFileExists(dmpFilePath))
			{
				cout << "\nError: File [" << dmpFilePath << "] dosen't exist. Skipping...";
				continue;
			}*/
			size_t uncompressedCdrmSize = cdrmHeader->getDataSize();
			auto_ptr<char> cdrmRawData(new char[uncompressedCdrmSize]);

			cdrmHeader->getData(cdrmRawData.get(), uncompressedCdrmSize);
			/*if (!isPacking)
			{
				fstream uncompressedCdrm(dmpFilePath, ios_base::out | ios_base::binary);
				if (!uncompressedCdrm.is_open())
				{
					cout << "\nError: Unable to open file [" << dmpFilePath << "]";
					continue;
				}
				uncompressedCdrm.write(cdrmRawData.get(), uncompressedCdrmSize);
			}*/
			hdr = ((uint32_t*)cdrmRawData.get())[0];
			switch (hdr)
			{
			case 0x39444350:	//PDC9
			{
				PCD9_Header* pcd9 = reinterpret_cast<PCD9_Header*>(cdrmRawData.get());
				string ddsFilePath = "";

				if (!isPacking)
				{
					ddsFilePath = subDir + "\\" + to_string(uh2_section) + ".dds";
					DDS dds(pcd9->ddsFlags, pcd9->height, pcd9->width, 0, pcd9->mipmap, pcd9->format, pcd9->getDataPtr(), pcd9->size);
					dds.serialization(ddsFilePath);
					cout << uh2_section << "," << ddsFilePath << endl;
				}
				else
				{
					ddsFilePath = path;
					DDS dds;
					if (!dds.deserialization(ddsFilePath))
					{
						errorCode = 1;
						break;
					}
					if (dds.getDataSize() != pcd9->size)
					{
						std::cout << "\nSize of old and new files dont match. This may corrupt files and render game unplayable.";
						std::cout << "\nTruncating for now";
					}
					size_t newImageSize = min(pcd9->size, dds.getDataSize());
					memcpy(pcd9->getDataPtr(), dds.getData(), newImageSize);
					pcd9->ddsFlags = dds.getFormat();
				}
			}
			break;
			default:
				break;
			}
			//	pack CDRM
			if (!errorCode && isPacking)
			{
				size_t newCdrmDataSize = 0;
				cdrmHeader->setData(cdrmRawData.get(), uncompressedCdrmSize, newCdrmDataSize);
				newCdrmDataSize += sizeof(CDRM_Header) + (sizeof(CDRM_DataChunks)*(cdrmHeader->count + 1) & 0xFFFFFFFE);
				uh2[uh2_section].setSize(newCdrmDataSize);
			}
		}
	}
	break;
	case 0xAC44:	// audio format of some sort.
	{
		cout << "AUDIO" << endl;
		string subDir = path + "\\" + to_string(id);
		string audioFilePath = path + "\\" + to_string(id) + ".audio";
		fstream localDump;
		if (!isPacking)
		{
			localDump.open(audioFilePath, ios_base::binary | ios_base::out);
			localDump.write(dataPtr, element.size);
		}
		else
		{
			localDump.open(audioFilePath, ios_base::binary | ios_base::in);
			size_t audioFileSize = getFileSize(&localDump);
			size_t dataToRead = min(element.size, audioFileSize);
			localDump.read(dataPtr, dataToRead);
			element.size = (uint32_t)dataToRead;
		}
	}
	break;
	default:
		errorCode = 1;
		break;
	}
	return errorCode;
}

int patch::unpack(int id, string path, bool silent)
{
	return process(id, -1, path, false, silent);
}

void patch::pack(int id, string path)
{
	process(id, -1, path, true);
}

int patch::pack(int id, int cdrmId, const string &path)
{
	return process(id, cdrmId, path, true);
}

void patch::printNameHashes()
{
	element *elements = tiger->getElements();
	for (uint32_t i = 0; i < tiger->getElementCount(); i++)
	{
		if (fileListHashMap.find(elements[i].nameHash) != fileListHashMap.end())
			cout << fileListHashMap[elements[i].nameHash] << "\n";
		else
			cout << hex << elements[i].nameHash << "\n";
	}
}
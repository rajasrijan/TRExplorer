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
#include "patch.h"
#include <algorithm>
#include <regex>
#include <assert.h>
#include "alldef.h"
#include "zlib.h"
#include "crc32.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#if defined(__unix__)
#include <dlfcn.h>
#include <sys/mman.h>
#define INVALID_FILE_DESCRIPTOR (-1)
#elif defined(_WIN32) || defined(WIN32)
#define INVALID_FILE_DESCRIPTOR (INVALID_HANDLE_VALUE)
#include <windows.h>
#define dlopen(x, y) LoadLibraryA(x)
#define dlsym(x, y) (void *)GetProcAddress((HMODULE)x, y)
#define dlclose(x) CloseHandle(x);
#define dlerror GetLastError
#endif

#if defined(__GNUC__)
#define LIBRARY_SUFFIX "lib"
#elif defined(_WIN32) || defined(WIN32)
#define LIBRARY_SUFFIX ""
#endif

#if defined(__unix__)
#define LIBRARY_EXTENSION ".so"
#elif defined(_WIN32) || defined(WIN32)
#define LIBRARY_EXTENSION ".dll"
#endif

size_t GetFileSize(int fd)
{
	struct stat st;
	if (fstat(fd, &st) != 0)
	{
		return 0;
	}
	//	get file size
	return st.st_size;
}

#ifdef WIN32
size_t GetFileSize(HANDLE fd)
{
	size_t fileSize = 0;
	((DWORD *)&fileSize)[0] = GetFileSize(fd, &(((DWORD *)&fileSize)[1]));
	return fileSize;
}
#endif

bool isDir(string pathname)
{
	struct stat info;

	if (stat(pathname.c_str(), &info) != 0)
		return false;
	else if (info.st_mode & S_IFDIR) // S_ISDIR() doesn't exist on my windows
		return true;
	else
		return false;
}

int makeDirectory(string path, bool recursive = false)
{
	if (!recursive && !isDir(path))
#ifdef __unix__
		return mkdir(path.c_str(), 0);
#else
		return mkdir(path.c_str());
#endif
	else
	{
		return 0;
	}
	return 0;
}

bool isFileExists(const std::string &file)
{
	struct stat buf;
	return (stat(file.c_str(), &buf) == 0);
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
	file->seekg(0, std::ios_base::end);
	std::streampos endPos = file->tellg();
	file->seekg(pos, std::ios_base::beg);
	return endPos;
}

patch::patch(const string &_path, bool silent) : tigerFilePath(_path), tiger(NULL)
{
#ifdef __unix__
	int mainTigerFile = open(tigerFilePath.c_str(), O_RDWR);
#else
	HANDLE mainTigerFile = CreateFileA(tigerFilePath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
#endif
	if (mainTigerFile == INVALID_FILE_DESCRIPTOR)
	{
		printf("Not able to open file [%s]. error(%d)", tigerFilePath.c_str(),
			   errno);
		exit(1);
	}
	fileHandles.push_back(mainTigerFile);

	size_t fileSz = GetFileSize(mainTigerFile);
	void *tigerFilePtr = nullptr;
#ifdef __unix__
	tigerFilePtr = mmap(nullptr, fileSz, PROT_READ | PROT_WRITE,
						MAP_SHARED, mainTigerFile, 0);
#else
	HANDLE mainTigerFileMapping = CreateFileMapping(mainTigerFile, NULL, PAGE_READWRITE, 0, 0, NULL);
	if (mainTigerFileMapping == INVALID_FILE_DESCRIPTOR)
	{
		printf("Failed to create mapping for file [%s]. error(%d)",
			   tigerFilePath.c_str(), errno);
		exit(1);
	}
	fileHandles.push_back(mainTigerFileMapping);
	tigerFilePtr = MapViewOfFile(mainTigerFileMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
#endif
	if (tigerFilePtr == nullptr)
	{
		printf("Not able to memory map file [%s]. error(%d)",
			   tigerFilePath.c_str(), errno);
		exit(1);
	}
	tigerPtrList.push_back(pair<void *, size_t>(tigerFilePtr, fileSz));
	tiger = new (tigerFilePtr) file_header();
	if (tiger->magic != TIGER_MAGIC)
	{
		printf("Invalid tiger file\n");
		exit(1);
	}
	tiger->printHeader();
	string basePath = tigerFilePath.substr(0, tigerFilePath.find(".000.tiger"));
	for (uint32_t i = 1; i < tiger->getFileCount(); i++)
	{
		char fileNo[32] = {0};
		sprintf(fileNo, ".%03d.tiger", i);
		tigerFilePath = basePath + fileNo;
#ifdef __unix__
		int tigerFile = open(tigerFilePath.c_str(), O_RDWR);
#else
		HANDLE tigerFile = CreateFileA(tigerFilePath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
#endif
		if (tigerFile == INVALID_FILE_DESCRIPTOR)
		{
			printf("Not able to open file [%s]. error(%d)",
				   tigerFilePath.c_str(), errno);
			exit(1);
		}
		fileHandles.push_back(tigerFile);
		fileSz = GetFileSize(tigerFile);
#ifdef __unix__
		tigerFilePtr = mmap(nullptr, fileSz, PROT_READ | PROT_WRITE,
							MAP_SHARED, tigerFile, 0);
#else
		HANDLE tigerFileMapping = CreateFileMapping(tigerFile, NULL, PAGE_READWRITE, 0, 0, NULL);
		if (tigerFileMapping == INVALID_FILE_DESCRIPTOR)
		{
			printf("Failed to create mapping for file [%s]. error(%d)",
				   tigerFilePath.c_str(), errno);
			exit(1);
		}
		fileHandles.push_back(tigerFileMapping);
		tigerFilePtr = MapViewOfFile(tigerFileMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
#endif
		if (tigerFilePtr == nullptr)
		{
			printf("Not able to memory map file [%s]. error(%d)",
				   tigerFilePath.c_str(), errno);
			exit(1);
		}
		tigerPtrList.push_back(pair<void *, size_t>(tigerFilePtr, fileSz));
		tiger->loadPtrToTigerFile(tigerFilePtr);
	}
	ifstream filelist;
	if (tiger->version == 0x04)
	{
		filelist.open("filelist2.txt");
	}
	else
	{
		filelist.open("filelist1.txt");
	}
	if (filelist.is_open())
	{
		char buffer[4096] = {0};
		string basePath = tiger->BasePath;
		do
		{
			filelist.getline(buffer, sizeof(buffer));
			string fullFilePath = basePath + buffer;
			if (fullFilePath.back() == '\r')	// needed on *nix
			{
				fullFilePath.pop_back();
			}
			uint32_t crc = crc32_hash(fullFilePath.data(),
									  fullFilePath.length());
			fileListHashMap[crc] = fullFilePath;
		} while (!filelist.eof());
		filelist.close();
	}
}

patch::~patch()
{
	//	Destroy in reverse order.
	for (auto fileMapPtrIt = tigerPtrList.rbegin();
		 fileMapPtrIt != tigerPtrList.rend(); fileMapPtrIt++)
	{
#ifdef __unix__
		munmap(fileMapPtrIt->first, fileMapPtrIt->second);
#else
		UnmapViewOfFile(fileMapPtrIt->first);
#endif
	}
	//	Destroy in reverse order.
	for (auto handleIt = fileHandles.rbegin(); handleIt != fileHandles.rend();
		 handleIt++)
	{
#ifdef __unix__
		close(*handleIt);
#else
		CloseHandle(*handleIt);
#endif
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

int patch::unpack(int id, string path, bool silent)
{
	return 0; //return process(id, -1, path, false, silent);
}

void patch::pack(int id, string path)
{
	//process(id, -1, path, true);
}

int patch::pack(int id, int cdrmId, const string &path)
{
	return 0; //return process(id, cdrmId, path, true);
}

element_t patch::getElement(uint32_t index)
{
	void *ele = tiger->getElement(index);
	element_t eleWrapper(ele, tiger->version);
	return eleWrapper;
}

int patch::getUHList(element_t &e, vector<void *> *pList)
{
	if (!pList)
	{
		return 1;
	}
	char *dataPtr = nullptr;
	void *elementPtr = e.getElement();
	if (tiger->version == 0x04)
	{
		dataPtr = (char *)((element_v2 *)elementPtr)->getDataPtr();
	}
	else
	{
		dataPtr = (char *)((element_v1 *)elementPtr)->getDataPtr();
	}
	//	process CDRM
	if (((uint32_t *)dataPtr)[0] == 0x16)
	{
		DRM_Header *drmHeader = new (dataPtr) DRM_Header;
		char *uh2 = (char *)drmHeader->getUnknownHeaders2();
		for (size_t uh2_section = 0;
			 uh2_section < drmHeader->getUnknownHeaders2Count();
			 uh2_section++)
		{
			if (tiger->version == 0x04)
			{
				pList->push_back(&((unknown_header2_v2 *)uh2)[uh2_section]);
			}
			else
			{
				pList->push_back(&((unknown_header2_v1 *)uh2)[uh2_section]);
			}
		}
	}
	return 0;
}

int patch::uncompressCDRM(void *pHdr, shared_ptr<char> &pData, size_t &sz)
{
	int ret = 0;
	if (tiger->version == 0x04)
	{
		ret = uncompressCDRM((unknown_header2_v2 *)pHdr, pData, sz);
	}
	else
	{
		ret = uncompressCDRM((unknown_header2_v1 *)pHdr, pData, sz);
	}
	return ret;
}

int patch::uncompressCDRM(unknown_header2_v1 *pHdr, shared_ptr<char> &pData,
						  size_t &sz)
{
	auto pCDRMHdr = pHdr->getCDRMPtr();
	if (!pCDRMHdr)
	{
		return 1;
	}
	sz = pCDRMHdr->getDataSize();
	pData = shared_ptr<char>(new char[sz]);
	return pCDRMHdr->getData(pData.get(), sz);
}

int patch::uncompressCDRM(unknown_header2_v2 *pHdr, shared_ptr<char> &pData,
						  size_t &sz)
{
	auto pCDRMHdr = pHdr->getCDRMPtr();
	sz = pCDRMHdr->getDataSize();
	pData = shared_ptr<char>(new char[sz]);
	return pCDRMHdr->getData(pData.get(), sz);
}

int patch::compressCDRM(void *pHdr, const char *pData, const size_t sz)
{
	int ret = 0;
	if (tiger->version == 0x04)
	{
		ret = compressCDRM((unknown_header2_v2 *)pHdr, pData, sz);
	}
	else
	{
		ret = compressCDRM((unknown_header2_v1 *)pHdr, pData, sz);
	}
	return ret;
}

int patch::compressCDRM(unknown_header2_v1 *pHdr, const char *pData,
						const size_t sz)
{
	int ret = 0;
	size_t szTotal = 0;
	CDRM_BlockFooter footer = *(pHdr->getCDRMFooter());
	CDRM_BlockFooter *oldFooter = pHdr->getCDRMFooter();
	ret = pHdr->getCDRMPtr()->setData(pData, sz, szTotal,
									  pHdr->size + footer.relative_offset);
	if (ret)
	{
		printf("Unable to set CDRM data\n");
		return ret;
	}
	pHdr->size = (uint32_t)(szTotal + sizeof(CDRM_Header));
	CDRM_BlockFooter *newFooter = pHdr->getCDRMFooter();
	newFooter->uiMagic = CDRM_BLOCK_FOOTER_MAGIC;
	newFooter->relative_offset = footer.relative_offset + (uint32_t)((int64_t)oldFooter - (int64_t)newFooter);
	printf("Size after compression [%zd]\n", szTotal);
	return ret;
}

int patch::compressCDRM(unknown_header2_v2 *pHdr, const char *pData,
						const size_t sz)
{
	int ret = 0;
	size_t szTotal = 0;
	CDRM_BlockFooter footer = *(pHdr->getCDRMFooter());
	CDRM_BlockFooter *oldFooter = pHdr->getCDRMFooter();
	ret = pHdr->getCDRMPtr()->setData(pData, sz, szTotal,
									  pHdr->size + footer.relative_offset);
	if (ret)
	{
		printf("Unable to set CDRM data\n");
		return ret;
	}
	pHdr->size = (uint32_t)(szTotal + sizeof(CDRM_Header));
	CDRM_BlockFooter *newFooter = pHdr->getCDRMFooter();
	newFooter->uiMagic = CDRM_BLOCK_FOOTER_MAGIC;
	newFooter->relative_offset = footer.relative_offset + (uint32_t)((int64_t)oldFooter - (int64_t)newFooter);
	printf("Size after compression [%zd]\n", szTotal);
	return ret;
}

int patch::getDataType(char *pData, size_t sz, CDRM_TYPES &type)
{
	int ret = 1;
	for (auto &plugin : pluginList)
	{
		if (plugin.second.pPluginInterface && plugin.second.pPluginInterface->check(pData, sz, type))
		{
			ret = 0;
			break;
		}
	}
	return ret;
}

int patch::decodeData(char *pData, size_t sz, char **ppDataOut, size_t &szOut,
					  CDRM_TYPES &type)
{
	int ret = 1;
	for (auto &plugin : pluginList)
	{
		if (plugin.second.pPluginInterface && plugin.second.pPluginInterface->check(pData, sz, type))
		{
			ret = plugin.second.pPluginInterface->unpack(pData, sz,
														 (void **)ppDataOut, szOut, type);
			break;
		}
	}
	return ret;
}

int patch::encodeData(char *pData, size_t sz, char **ppDataOut,
					  size_t &szOut, CDRM_TYPES &type)
{
	int ret = 1;
	for (auto &plugin : pluginList)
	{
		ret = plugin.second.pPluginInterface->pack(pData, sz,
												   (void **)ppDataOut, szOut, type);
		if (!ret)
		{
			break;
		}
	}
	return ret;
}

int patch::decodeAndSaveToFile(void *pHdr, string fileName)
{
	int ret = 0;
	if (tiger->version == 0x04)
	{
		decodeAndSaveToFile((CDRM_Header *)pHdr, fileName);
	}
	else
	{
		decodeAndSaveToFile((CDRM_Header *)pHdr, fileName);
	}
	return ret;
}

int patch::decodeAndSaveToFile(CDRM_Header *pHdr, string fileName)
{
	int ret = 0;
	int bytesWritten = 0;
	size_t sz = 0, szOut = 0;
	shared_ptr<char> pData;
	char *pDecodedData = nullptr;
	CDRM_TYPES type = CDRM_TYPE_UNKNOWN;
	int hFile = -1;
	if (!pHdr)
	{
		return 1;
	}
	ret = uncompressCDRM(pHdr, pData, sz);
	if (ret)
	{
		printf("Unable to uncompress CDRM");
		goto exit;
	}
	ret = decodeData(pData.get(), sz, &pDecodedData, szOut, type);
	if (ret)
	{
		printf("Unable to decode CDRM");
		goto exit;
	}
	hFile = open(fileName.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (hFile == -1)
	{
		printf("Unable to open file");
		ret = 1;
		goto exit;
	}
	bytesWritten = write(hFile, pDecodedData, (int)szOut);
	if (bytesWritten < 0)
	{
		printf("Unable to write file");
		ret = errno;
		goto exit;
	}
	else
	{
		ret = 0;
	}
exit:
	if (pDecodedData)
	{
		delete pDecodedData;
		pDecodedData = nullptr;
	}
	if (hFile != -1)
	{
		close(hFile);
	}
	return ret;
}

int patch::encodeAndCompress(void *pHdr, string fileName)
{
	int ret = 0;
	if (tiger->version == 0x04)
	{
		ret = encodeAndCompress((unknown_header2_v2 *)pHdr, fileName);
	}
	else
	{
		ret = encodeAndCompress((unknown_header2_v1 *)pHdr, fileName);
	}
	return ret;
}

int patch::encodeAndCompress(unknown_header2_v1 *pHdr, string fileName)
{
	int ret = 0;
	size_t sz = 0, szFile = 0;
	char *pData = nullptr;
	int bytesRead = 0;
	shared_ptr<char> pRawData;
	CDRM_TYPES type = CDRM_TYPE_UNKNOWN;
	int hFile = -1;
	if (!pHdr)
	{
		return 1;
	}
	hFile = open(fileName.c_str(), O_RDWR);
	if (hFile == -1)
	{
		printf("Unable to open file");
		ret = 1;
		goto exit;
	}
	//	get file size
	szFile = GetFileSize(hFile);
	// allocate sufficient memory.
	pRawData = shared_ptr<char>(new char[szFile]);
	bytesRead = (int)read(hFile, pRawData.get(), (int)szFile);
	if (bytesRead < 0)
	{
		printf("Unable to write file");
		ret = errno;
		goto exit;
	}
	else
	{
		ret = 0;
	}
	ret = encodeData(pRawData.get(), szFile, &pData, sz, type);
	if (ret)
	{
		printf("Unable to encode data. No encoder found\n");
		goto exit;
	}
	ret = compressCDRM(pHdr, pData, sz);
	if (ret)
	{
		printf("Unable to compress CDRM\n");
		goto exit;
	}
exit:
	if (pData)
	{
		delete pData;
		pData = nullptr;
	}
	if (hFile != -1)
	{
		close(hFile);
	}
	return ret;
}

int patch::encodeAndCompress(unknown_header2_v2 *pHdr, string fileName)
{
	int ret = 0;
	size_t sz = 0, szFile = 0;
	char *pData = nullptr;
	int bytesRead = 0;
	shared_ptr<char> pRawData;
	CDRM_TYPES type = CDRM_TYPE_UNKNOWN;
	int hFile = -1;
	if (!pHdr)
	{
		return 1;
	}
	hFile = open(fileName.c_str(), O_RDWR);
	if (hFile == -1)
	{
		printf("Unable to open file");
		ret = 1;
		goto exit;
	}
	//	get file size
	szFile = GetFileSize(hFile);
	// allocate sufficient memory.
	pRawData = shared_ptr<char>(new char[szFile]);

	bytesRead = (int)read(hFile, pRawData.get(), (int)szFile);
	if (bytesRead < 0)
	{
		printf("Unable to read file");
		ret = errno;
		goto exit;
	}
	else
	{
		ret = 0;
	}
	ret = encodeData(pRawData.get(), szFile, &pData, sz, type);
	if (ret)
	{
		printf("Unable to encode data. No encoder found\n");
		goto exit;
	}
	ret = compressCDRM(pHdr, pData, sz);
	if (ret)
	{
		printf("Unable to compress CDRM\n");
		goto exit;
	}
exit:
	if (pData)
	{
		delete pData;
		pData = nullptr;
	}
	if (hFile != -1)
	{
		close(hFile);
	}
	return ret;
}
size_t patch::getElementCount()
{
	return tiger->getElementCount();
}

vector<pair<string, Plugin>> pluginList;

int loadPlugins()
{
	pluginList.clear();
	ifstream pluginListFile("PluginList.txt");
	if (!pluginListFile.is_open())
	{
		return 1;
	}
	while (!pluginListFile.eof())
	{
		Plugin p;
		int errCode = 0;
		char pluginName[NAME_LENGTH] = {0};
		char plugin[NAME_LENGTH] = {0};
		//	read plugin name
		pluginListFile >> plugin;
		//	append library extension
		sprintf(pluginName, "./%s%s%s", LIBRARY_SUFFIX, plugin,
				LIBRARY_EXTENSION);

		//	try to load library.
		p.hPluginDll = dlopen(pluginName, RTLD_NOW);
		if (p.hPluginDll == NULL)
		{
			printf("failed to open \"%s\". Reason [%s]\n", pluginName,
				   dlerror());
			continue;
		}
		//	try to get create and destroy handles.
		p.pfnCreate = dlsym(p.hPluginDll, "createPluginInterface");
		p.pfnDestroy = dlsym(p.hPluginDll, "destroyPluginInterface");
		if (p.pfnCreate == NULL || p.pfnDestroy == NULL)
		{
			printf("failed to load interface for \"%s\". Reason [%s]\n",
				   pluginName, dlerror());
			dlclose(p.hPluginDll);
			continue;
		}
		//	create interface
		errCode = ((createPluginInterface)p.pfnCreate)(&p.pPluginInterface);
		if (errCode)
		{
			dlclose(p.hPluginDll);
			continue;
		}
		pluginList.push_back(pair<string, Plugin>(pluginName, p));
	}
	return 0;
}

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

#include <cstdio>
#include <string>
#include "tiger.h"
#include "PluginInterface.h"
#include "element.h"
#ifdef WIN32
#define UNICODE
#include <windows.h>
#endif

struct Plugin
{
    void *hPluginDll;
    void *pfnCreate, *pfnDestroy;
    PluginInterface* pPluginInterface;
};
extern vector<pair<string, Plugin>> pluginList;
int loadPlugins();

class patch
{
    vector<pair<void*,size_t> > tigerPtrList;
#ifdef __unix__
	vector<int> fileHandles;
#else
	vector<HANDLE> fileHandles;
#endif
	file_header *tiger;
    string tigerFilePath;
public:
    patch(const string &_path, bool silent = false);
    ~patch();
    int unpack(int id, string path = "output", bool silent = true);
    int unpackAll(string path = "output");
    void pack(int id, string path = "output");
    int pack(int id, int cdrmId, const string &path);
    size_t getElementCount();
    element_t getElement(uint32_t index);
    int getUHList(element_t & e, vector<void*> *pList);
    int uncompressCDRM(void* pHdr, shared_ptr<char>& pData, size_t& sz);
    int uncompressCDRM(unknown_header2_v1* pHdr, shared_ptr<char>& pData, size_t& sz);
    int uncompressCDRM(unknown_header2_v2* pHdr, shared_ptr<char>& pData, size_t& sz);
    int compressCDRM(void * pHdr, const char* pData, const size_t sz);
    int compressCDRM(unknown_header2_v1 * pHdr, const char* pData, const size_t sz);
    int compressCDRM(unknown_header2_v2 * pHdr, const char* pData, const size_t sz);
    int getDataType(char* pData, size_t sz, CDRM_TYPES &type);
    int decodeData(char* pData, size_t sz, char** ppDataOut, size_t &szOut, CDRM_TYPES &type);
    int encodeData(char* pData, size_t sz, char** ppDataOut, size_t &szOut, CDRM_TYPES &type);
    int decodeAndSaveToFile(void* pHdr, string fileName);
    int decodeAndSaveToFile(CDRM_Header* pHdr, string fileName);
    int encodeAndCompress(void* pHdr, string fileName);
    int encodeAndCompress(unknown_header2_v1* pHdr, string fileName);
    int encodeAndCompress(unknown_header2_v2* pHdr, string fileName);
};

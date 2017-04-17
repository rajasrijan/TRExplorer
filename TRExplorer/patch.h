#pragma once
#include <Windows.h>
#include <stdio.h>
#include <strsafe.h>
#include "tiger.h"

class patch
{
	vector<void*> tigerPtrList;
	vector<HANDLE> fileHandles;
	file_header *tiger;
	string tigerFilePath;
	map<uint32_t, string> fileListHashMap;
public:
	patch(string _path, bool silent = false);
	~patch();
	int unpack(int id, string path = "output", bool silent = true);
	int unpackAll(string path = "output");
	int process(int id, int cdrmId, const string& path, bool isPacking, bool silent = true);
	void pack(int id, string path = "output");
	int pack(int id, int cdrmId, const string &path);
	void printNameHashes();
};


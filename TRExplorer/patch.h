#pragma once
#include "tiger.h"

class patch
{
	//first = file id, second = file array.
	map<uint32_t, vector<fstream*>> tigerFiles;
	fileHeader header;
	string tigerFilePath;
	//fstream pfile;
	//vector<element> elements;
public:
	patch(string _path);
	~patch();
	void unpack(int id, string path = "output");
	void unpackAll(string path = "output");
	void process(int id, string path, bool isPacking);
	void pack(int id, string path = "output");
private:
	int findEmptyCDRM(size_t sizeHint, uint32_t &offset, int &file, int &base);
	int getCDRM(uint32_t offset,CDRMHeader& cdrmHeader);
};


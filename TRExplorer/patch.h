#pragma once
#include "tiger.h"

class patch
{
	file_header header;
	string filePath;
	fstream pfile;
	vector<element> elements;
public:
	patch(string _path);
	~patch();
};


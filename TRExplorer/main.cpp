#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <stdio.h>
#include <memory>
#include <map>
#include <string>
#include <string.h>
#include "DDS.h"
#include "tiger.h"
#include "patch.h"

using namespace std;

int main(int argc, char *argv[])
{
	system("title rajasrijan's tiger decoder.");
	patch p(argv[1]);
	//p.unpackAll();
	//p.unpack(0);
	//p.unpack(212);
	//p.pack(212);
	if (argc == 2)
	{
		TIGER file(argv[1]);
		cout << "\nValid IDs range from 0 to (DRMs-1)\n\n";
		return 0;
	}
	else if (argc == 4)
	{
		string operation = argv[2];
		TIGER file(argv[1]);

		if (operation == "pack")
		{
			uint32_t nameHash = 0;
			sscanf(argv[3], "%x", &nameHash);
			file.pack(nameHash, "tmp");
		}
		else if (operation == "unpack")
		{
			uint32_t id = 0;
			sscanf(argv[3], "%d", &id);
			file.unpack(id, "tmp");
		}
		else
		{
			cout << "\nInvalid operation.\n";
			exit(-1);
		}
	}
	else
	{
		cout << "Format:\n\n" << argv[0] << " <LOCATION> <OPERATION> <ID>\n\n";
		cout << "\tLOCATION\tLocation of bigfiles.\n";
		cout << "\tOPERATION\tUNPACK/PACK\n";
		cout << "\tID\t\tID in the .tiger file to pack/unpack\n\n";
		return -1;
	}
	return 0;
}

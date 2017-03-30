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
	int ret = 0;
	system("title rajasrijan's tiger decoder.");

	if (argc >= 2)
	{
		string tigerFileLocation = argv[1];
		string operation = argv[2];
		patch p(tigerFileLocation, true);
		if (operation == "info")
		{
			p.printNameHashes();
		}
		else if (argc == 5)
		{
			int drmIndex = atoi(argv[3]);
			string outputLocation = argv[4];
			if (operation == "unpack")
			{
				ret = p.unpack(drmIndex, outputLocation, true);
			}
			else if (operation == "pack")
			{
				p.pack(drmIndex, outputLocation);
			}
		}
		else
		{
			cout << "Format:\n\n" << argv[0] << " <LOCATION> <OPERATION> <ID>\n\n";
			cout << "\tLOCATION\tLocation of bigfiles.\n";
			cout << "\tOPERATION\tUNPACK/PACK\n";
			cout << "\tID\t\tID in the .tiger file to pack/unpack\n\n";
			return 1;
		}
	}
	return ret;
}

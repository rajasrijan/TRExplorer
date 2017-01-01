#define logline(x) cout<<__FUNCTION__<<":"<<__LINE__<<"::"<<x<<endl

#include <iostream>
#include <fstream>
#include <vector>
#include <tuple>
#include <direct.h>

using namespace std;

typedef tuple<size_t, size_t> cdrmPointer;
size_t cdrmIdCounter = 0;
vector<cdrmPointer> cdrmList;

struct iHeader
{
	uint32_t magic;
	uint32_t version;
	uint32_t NumberOfFiles;
	uint32_t count, d4;
	char BasePath[32];
};
//Entry stucture for version 4
struct iEntryV4
{
	uint32_t   dwHash; //Hash of filename
	uint32_t   dwLanguage;
	uint32_t   dwSize;
	uint32_t   dwZSize; //Posible
	uint32_t   dwFlags; //or two uint16_t
	uint32_t   dwOffset;
};
struct iDRMHeader
{
	uint32_t   dwVersion; //16
	uint32_t   dwStrLength1;
	uint32_t   dwStrLength2;
	uint32_t   dwNull1;
	uint32_t   dwNull2;
	uint32_t   dwNull3;
	uint32_t   dwTotalSections;
	uint32_t   dwLocaleID;
};
struct iDRMEntry
{
	uint32_t   dwUnknown1;
	uint32_t   dwType;
	uint32_t   dwResourceID; //??
	uint32_t   dwSectionID; //??
	uint32_t   dwLanguageID;
};
struct iDRMSubEntry
{
	uint32_t   dwUnknown1;
	uint32_t   dwNull;
	uint32_t   dwFlags; // or two uint16_t
	uint32_t   dwOffset;
	uint32_t   dwLanguageID;
	uint32_t   dwSize;
	//uint32_t   dwUnknown2;
};

string getFileName(string& path)
{
	size_t start = path.find_last_of("\\");
	return string(path.begin() + (start + 1), path.end());
}

void printCDRMList()
{
	for (auto cdrmItr : cdrmList)
	{
		cout << "ID: " << get<0>(cdrmItr) << ", Offset: " << get<1>(cdrmItr) << "\n";
	}
}

int getFileLength(fstream &file, size_t &length)
{
	std::streamoff g = file.tellg();
	file.seekg(0, file.end);
	length = file.tellg();
	file.seekg(g);
	return 0;
}
template<typename t>
void readStructureFromFile(fstream &file, t &structure)
{
	file.read((char*)&structure, sizeof(t));
}
template<typename t>
void readStructuresFromFile(fstream &file, t *structure, size_t count)
{
	if (structure == NULL)
	{
		return;
	}
	file.read((char*)structure, sizeof(t)*count);
}

int main()
{
	size_t totalSizeTillNow = 0;
	string tigerFilePath = "C:\\Program Files (x86)\\Rise of the Tomb Raider\\bigfile.000.tiger";
	//string tigerFilePath = "test.txt";
	fstream tigerFile(tigerFilePath, ios_base::in | ios_base::binary);
	if (!tigerFile.is_open())
	{
		cout << "failed to open file";
		return 0;
	}
	iHeader header;
	readStructureFromFile(tigerFile, header);
	vector<iEntryV4> EntryV4(header.count);
	readStructuresFromFile(tigerFile, EntryV4.data(), EntryV4.size());
	int index = 0, size = EntryV4.size();
	for (auto entry : EntryV4)
	{
		cout << ++index << " of " << size << endl;
		int type = entry.dwFlags & 0xFF;
		if (type == 0)
		{
			tigerFile.seekg(entry.dwOffset);
			iDRMHeader drm;
			readStructureFromFile(tigerFile, drm);
			vector<iDRMEntry> DRMEntry(drm.dwTotalSections);
			tigerFile.seekg(drm.dwStrLength1 + drm.dwStrLength2, SEEK_CUR);
			vector<iDRMSubEntry> DRMSubEntry(drm.dwTotalSections);
			readStructuresFromFile(tigerFile, DRMEntry.data(), DRMEntry.size());
			readStructuresFromFile(tigerFile, DRMSubEntry.data(), DRMSubEntry.size());
			for (auto dentry : DRMSubEntry)
			{
				cout << hex << dentry.dwOffset << "\n";
				totalSizeTillNow += dentry.dwSize;
			}
		}
		totalSizeTillNow += entry.dwSize;
	}
	cout << "\n\n" << "Total size: " << totalSizeTillNow << "\n";
	return 0;
}
#include "patch.h"
#include <algorithm>
#include "alldef.h"


patch::patch(string _path) :filePath(_path)
{
	int main_offset, drm_offset;
	pfile.open(_path, ios_base::in | ios_base::out | ios_base::binary);
	if (!pfile.is_open())
	{
		cout << "Error Opening patch file\n";
		exit(-1);
	}

	main_offset = 0;
	header = fileHeader(0, &pfile);
	header.get().printHeader();

	//load table
	//vector<elementHeader> elements = header.it();

	//system("mkdir patch_drms");
	//for (size_t i = 212; i < 213/*elements.size()*/; i++)
	//{
	//	uint32_t offset = elements[i].get().offset & 0xFFFFF800;
	//	uint32_t base = elements[i].get().offset & 0x000007FF;
	//	int size = elements[i].get().size;

	//	DRMHeader drmHeader(offset, &pfile);
	//	drmHeader.get().printHeader();
	//	DRM_Header &drmHdr = drmHeader.get();

	//	vector<unknownHeader2> uh2 = drmHeader.it((drmHdr.SectionCount*sizeof(unknown_header1)) + drmHdr.strlen1 + drmHdr.strlen2);
	//	system(("mkdir patch_drms\\" + to_string(i)).c_str());

	//	for (size_t j = 0; j < 1/*uh2.size()*/; j++)
	//	{
	//		string cdrmFilePath = ".\\patch_drms\\" + to_string(i) + "\\" + to_string(j) + ".cdrm";
	//		uint32_t offset = uh2[j].get().offset & 0xFFFFF800;
	//		uint32_t base = (uh2[j].get().offset & 0x000007F0) >> 4;
	//		uint32_t fileNo = uh2[j].get().offset & 0x0000000F;
	//		if (base != header.get().fileId)
	//			continue;
	//		int size = uh2[j].get().size;

	//		CDRMHeader head(offset, &pfile);
	//		
	//		head.get().printHeader();

	//		vector<CDRMBlockHeader> bh = head.it();
	//		if (bh.size() % 2)
	//			pfile.seekg(8, ios_base::cur);
	//		fstream cdrmFile(cdrmFilePath, ios_base::binary | ios_base::out);
	//		pfile.seekg(offset);
	//		for (size_t z = 0; z < size; z += 0x800)
	//		{
	//			char d[0x800] = { 0 };
	//			pfile.read(d, 0x800);
	//			cdrmFile.write(d, 0x800);
	//		}

	//	}
	//}
}


patch::~patch()
{
}

void patch::unpack(int id, string path)
{
	//load table
	vector<elementHeader> elements = header.it();

	if (id<0 || id >(int)elements.size())
		return;
	element &element = elements[id].get();
	system(("mkdir " + path).c_str());

	uint32_t offset = element.offset & 0xFFFFF800;
	uint32_t base = element.offset & 0x000007FF;
	int size = element.size;

	DRMHeader drmHeader(offset, &pfile);
	DRM_Header &drmHdr = drmHeader.get();
	drmHdr.printHeader();

	vector<unknownHeader2> uh2 = drmHeader.it((drmHdr.SectionCount*sizeof(unknown_header1)) + drmHdr.strlen1 + drmHdr.strlen2);
	string subDir = path + "\\" + to_string(id);
	system(("mkdir " + subDir).c_str());

	for (size_t j = 0; j < uh2.size(); j++)
	{
		string cdrmFilePath = subDir + "\\" + to_string(j) + ".cdrm";
		unknown_header2 &uheader2 = uh2[j].get();
		uint32_t offset = uheader2.offset & 0xFFFFF800;
		uint32_t base = (uheader2.offset & 0x000007F0) >> 4;
		uint32_t fileNo = uheader2.offset & 0x0000000F;
		if (base != header.get().fileId)
			continue;
		int size = uheader2.size;

		CDRMHeader head(offset, &pfile);
		head.get().printHeader();

		vector<CDRMBlockHeader> bh = head.it();
		fstream cdrmFile(cdrmFilePath, ios_base::binary | ios_base::out);
		if (cdrmFile.is_open())
		{
			pfile.seekg(offset);
			for (size_t z = 0; z < size; z += 0x800)
			{
				char d[0x800] = { 0 };
				pfile.read(d, 0x800);
				cdrmFile.write(d, 0x800);
			}
		}
	}
}

void patch::pack(int id, string path)
{
	map<int, string> cdrmToPack;
	//load table
	vector<elementHeader> elements = header.it();

	if (id<0 || id > elements.size())
		return;
	element &element = elements[id].get();
	system(("mkdir " + path).c_str());

	uint32_t offset = element.offset & 0xFFFFF800;
	uint32_t base = element.offset & 0x000007FF;
	int size = element.size;

	DRMHeader drmHeader(offset, &pfile);
	DRM_Header &drmHdr = drmHeader.get();
	drmHdr.printHeader();

	vector<unknownHeader2> uh2 = drmHeader.it((drmHdr.SectionCount*sizeof(unknown_header1)) + drmHdr.strlen1 + drmHdr.strlen2);
	string subDir = path + "\\" + to_string(id);
	system(("mkdir " + subDir).c_str());



	for (size_t j = 0; j < uh2.size(); j++)
	{
		string cdrmFilePath = subDir + "\\" + to_string(j) + ".cdrm";
		ifstream cdrmFile(cdrmFilePath);
		if (cdrmFile.is_open())
		{
			cdrmToPack[j] = cdrmFilePath;
		}
	}
	cout << "Files found to be packed" << endl;
	for (auto it = cdrmToPack.begin(); it != cdrmToPack.end(); it++)
	{
		cout << "(" << it->first << ", " << it->second << ")" << endl;
	}
	/*for (size_t j = 0; j < uh2.size(); j++)
	{
		string cdrmFilePath = subDir + "\\" + to_string(j) + ".cdrm";
		unknown_header2 &uheader2 = uh2[j].get();
		uint32_t offset = uheader2.offset & 0xFFFFF800;
		uint32_t base = (uheader2.offset & 0x000007F0) >> 4;
		uint32_t fileNo = uheader2.offset & 0x0000000F;
		if (base != header.get().fileId)
			continue;
		int size = uheader2.size;

		CDRMHeader head(offset, &pfile);
		head.get().printHeader();

		vector<CDRMBlockHeader> bh = head.it();
		fstream cdrmFile(cdrmFilePath, ios_base::binary | ios_base::out);
		if (cdrmFile.is_open())
		{
			pfile.seekg(offset);
			for (size_t z = 0; z < size; z += 0x800)
			{
				char d[0x800] = { 0 };
				pfile.read(d, 0x800);
				cdrmFile.write(d, 0x800);
			}
		}
	}*/
}

int patch::findEmptyCDRM(size_t sizeHint, uint32_t &offset, int &file, int &base)
{
	fstream file;
}
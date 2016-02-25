#include "patch.h"
#include <algorithm>
#include "alldef.h"


patch::patch(string _path) :filePath(_path)
{
	int main_offset, drm_offset;
	CDRM_Header newHeader;
	shared_ptr<char> cdrmData;
	unknown_header2 u;
	int cdrmSize = 0;
	pfile.open(_path, ios_base::in | ios_base::out | ios_base::binary);
	if (!pfile.is_open())
	{
		cout << "Error Opening patch file\n";
		exit(-1);
	}

	//TigerHelper tiger(0, &pfile);

	main_offset = 0;
	pfile.read((char*)&header, sizeof(header));
	header.printHeader();

	//load table
	elements = vector<element>(header.count);
	int tabel_size = header.count*sizeof(element);
	pfile.read((char*)elements.data(), tabel_size);
	
	system("mkdir patch_drms");
	for (size_t i = 0; i < elements.size(); i++)
	{
		uint32_t offset = elements[i].offset & 0xFFFFF800;
		uint32_t base = elements[i].offset & 0x000007FF;
		int size = elements[i].size;

		//pfile.seekg(offset);
		///*Write DRM*/
		//{
		//	string drmFilePath = ".\\patch_drms\\" + to_string(i) + ".drm";
		//	fstream output(drmFilePath, ios_base::binary | ios_base::out);
		//	while (size > 0)
		//	{
		//		char data[0x800] = { 0 };
		//		pfile.read(data, std::min(0x800, size));
		//		output.write(data, 0x800);
		//		size -= 0x800;
		//	}
		//	output.close();
		//}
		pfile.seekg(offset);
		drm_offset = offset;
		//DRM_Header drmHeader;
		DRMHeader drmHeader(offset, &pfile);
		drmHeader.load(&pfile);
		
		drmHeader.printHeader();

		vector<unknown_header1> uh1(drmHeader.SectionCount);
		pfile.read((char*)uh1.data(), sizeof(unknown_header1)*uh1.size());
		/*skip string in between*/
		pfile.seekg(drmHeader.strlen1 + drmHeader.strlen2, ios_base::cur);
		drm_offset = pfile.tellg();
		vector<unknown_header2> uh2(drmHeader.SectionCount);
		pfile.read((char*)uh2.data(), sizeof(unknown_header2)*uh2.size());
		continue;
		u = uh2[0];
		system(("mkdir patch_drms\\" + to_string(i)).c_str());

		for (size_t j = 0; j < 1/*uh2.size()*/; j++)
		{
			string cdrmFilePath = ".\\patch_drms\\" + to_string(i) + "\\" + to_string(j) + ".cdrm";
			uint32_t offset = uh2[j].offset & 0xFFFFF800;
			uint32_t base = (uh2[j].offset & 0x000007F0) >> 4;
			uint32_t fileNo = uh2[j].offset & 0x0000000F;
			if (base != header.fileId)
				continue;
			int size = uh2[j].size;

			pfile.seekg(offset);
			/*Write CDRM*/
			{
				fstream output(cdrmFilePath, ios_base::binary | ios_base::out);
				int paddedSize = ALIGN_TO(size, 0x800);/*CDRM is stored in 0x800 boundry. will include footer too*/
				while (paddedSize > 0)
				{
					char data[0x800] = { 0 };
					pfile.read(data, std::min(0x800, paddedSize));
					output.write(data, 0x800);
					paddedSize -= 0x800;
				}
				output.close();
			}
			pfile.seekg(offset);
			CDRM_Header head;
			head.load(&pfile);
			head.printHeader();
			newHeader = head;

			vector<CDRM_BlockHeader> bh(head.count);
			pfile.read((char*)bh.data(), sizeof(CDRM_BlockHeader)*bh.size());
			if (bh.size() % 2)
				pfile.seekg(8, ios_base::cur);
			//system(("mkdir patch_drms\\" + to_string(i) + "\\" + to_string(j)).c_str());
			offset = pfile.tellg();
			/*File save*/
			int usize = 0;
			for (size_t k = 0; k < bh.size(); k++)
			{
				size = bh[k].compressedSize;
				usize = bh[k].uncompressedSize;
			}
			pfile.seekg(offset);

			string filePath = cdrmFilePath + ".raw";
			fstream output(filePath, ios_base::binary | ios_base::out);

			shared_ptr<char> data(new char[size]);
			shared_ptr<char> udata(new char[usize]);
			cdrmData = udata;
			cdrmSize = usize;
			pfile.read(data.get(), size);
			uLongf destLen = usize;
			uncompress((Bytef*)udata.get(), &destLen, (Bytef*)data.get(), size);
			output.write(udata.get(), destLen);
			output.close();
			offset += destLen;
			offset = ALIGN_TO(offset, 0x800);
		}
	}

	string newFilePath = _path.substr(0, _path.find_last_of("\\")) + "\\bigfile.004.tiger";
	fstream newFile(newFilePath, ios_base::out | ios_base::binary);

	newFile.write((char*)&newHeader, sizeof(newHeader));
	CDRMBlockHeader blkHdr;
	CDRM_BlockFooter blkFtr;
	//header
	blkHdr.setOffset(sizeof(CDRM_Header));
	blkHdr.setFile(&newFile);
	blkHdr.get().blockType = 1;
	blkHdr.get().compressedSize = cdrmSize;
	blkHdr.get().uncompressedSize = cdrmSize;
	//footer
	((uint32_t*)(blkFtr.magic))[0] = 'TXEN';
	blkFtr.relative_offset = 0x800 - (cdrmSize % 0x800);


	blkHdr.serialize();
	for (size_t i = 0; i < 8; i++)
	{
		newFile << 'P';
	}
	//newFile.write((char*)&blkHdr, sizeof(CDRM_BlockHeader));
	//newFile.write((char*)&blkHdr, sizeof(CDRM_BlockHeader));
	newFile.write(cdrmData.get(), cdrmSize);
	int off = newFile.tellp();
	int diff = 0x10 - (off % 0x10);
	for (size_t i = 0; i < diff; i++)
		newFile << '\x00';

	blkFtr.relative_offset = 0x800 - ((off) % 0x800);
	newFile.write((char*)&blkFtr, sizeof(CDRM_BlockFooter));
	u.offset = 4;
	u.size = cdrmSize;
	pfile.seekp(drm_offset);
	pfile.write((char*)&u, sizeof(unknown_header2));
	header.NumberOfFiles = 5;
	pfile.seekp(0);
	header.save(&pfile);
	char data[0x800];
	newFile.write(data, 0x800);
}


patch::~patch()
{
}
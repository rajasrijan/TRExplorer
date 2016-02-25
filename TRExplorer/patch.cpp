#include "patch.h"
#include <algorithm>
#include "alldef.h"


patch::patch(string _path) :filePath(_path)
{
	int main_offset, drm_offset;
	//CDRM_Header newHeader;
	//shared_ptr<char> cdrmData;
	//unknown_header2 u;
	int cdrmSize = 0;
	pfile.open(_path, ios_base::in | ios_base::out | ios_base::binary);
	if (!pfile.is_open())
	{
		cout << "Error Opening patch file\n";
		exit(-1);
	}

	//TigerHelper tiger(0, &pfile);

	main_offset = 0;
	fileHeader header(0,&pfile);
	//pfile.read((char*)&header, sizeof(header));
	//header.printHeader();

	//load table
	vector<elementHeader> elements = header.it();
	/*int tabel_size = header.get().count*sizeof(element);
	pfile.read((char*)elements.data(), tabel_size);*/

	system("mkdir patch_drms");
	for (size_t i = 212; i < 213/*elements.size()*/; i++)
	{
		uint32_t offset = elements[i].get().offset & 0xFFFFF800;
		uint32_t base = elements[i].get().offset & 0x000007FF;
		int size = elements[i].get().size;

		//pfile.seekg(offset);
		//drm_offset = offset;
		//DRM_Header drmHeader;
		DRMHeader drmHeader(offset, &pfile);
		drmHeader.get().printHeader();
		DRM_Header &drmHdr = drmHeader.get();
		//drmHeader.it((drmHdr.SectionCount*sizeof(unknown_header1)) + drmHdr.strlen1 + drmHdr.strlen2);
		//vector<unknown_header1> uh1(drmHeader.get().SectionCount);
		//pfile.read((char*)uh1.data(), sizeof(unknown_header1)*uh1.size());
		///*skip string in between*/
		//pfile.seekg(drmHeader.get().strlen1 + drmHeader.get().strlen2, ios_base::cur);
		//drm_offset = pfile.tellg();
		/*vector<unknown_header2> uh2(drmHeader.get().SectionCount);
		pfile.read((char*)uh2.data(), sizeof(unknown_header2)*uh2.size());*/
		vector<unknownHeader2> uh2 = drmHeader.it((drmHdr.SectionCount*sizeof(unknown_header1)) + drmHdr.strlen1 + drmHdr.strlen2);
		system(("mkdir patch_drms\\" + to_string(i)).c_str());

		for (size_t j = 0; j < 1/*uh2.size()*/; j++)
		{
			string cdrmFilePath = ".\\patch_drms\\" + to_string(i) + "\\" + to_string(j) + ".cdrm";
			uint32_t offset = uh2[j].get().offset & 0xFFFFF800;
			uint32_t base = (uh2[j].get().offset & 0x000007F0) >> 4;
			uint32_t fileNo = uh2[j].get().offset & 0x0000000F;
			if (base != header.get().fileId)
				continue;
			int size = uh2[j].get().size;

			pfile.seekg(offset);
			/*Write CDRM*/
			//{
			//	fstream output(cdrmFilePath, ios_base::binary | ios_base::out);
			//	int paddedSize = ALIGN_TO(size, 0x800);/*CDRM is stored in 0x800 boundry. will include footer too*/
			//	while (paddedSize > 0)
			//	{
			//		char data[0x800] = { 0 };
			//		pfile.read(data, std::min(0x800, paddedSize));
			//		output.write(data, 0x800);
			//		paddedSize -= 0x800;
			//	}
			//	output.close();
			//}
			pfile.seekg(offset);
			CDRMHeader head(offset, &pfile);
			head.get().printHeader();
			//head.it();
			//vector<CDRM_BlockHeader> bh(head.get().count);
			auto bh = head.it();
			//pfile.read((char*)bh.data(), sizeof(CDRM_BlockHeader)*bh.size());
			if (bh.size() % 2)
				pfile.seekg(8, ios_base::cur);
			//system(("mkdir patch_drms\\" + to_string(i) + "\\" + to_string(j)).c_str());
			offset = pfile.tellg();
			/*File save*/
			int usize = 0;
			for (size_t k = 0; k < bh.size(); k++)
			{
				size = bh[k].get().compressedSize;
				usize = bh[k].get().uncompressedSize;
			}
			pfile.seekg(offset);

			string filePath = cdrmFilePath + ".raw";
			fstream output(filePath, ios_base::binary | ios_base::out);

			shared_ptr<char> data(new char[size]);
			shared_ptr<char> udata(new char[usize]);
			//cdrmData = udata;
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
}


patch::~patch()
{
}
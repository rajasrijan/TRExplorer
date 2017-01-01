#define _CRT_SECURE_NO_WARNINGS
#include "patch.h"
#include <algorithm>
#include <regex>
#include <assert.h>
#include "alldef.h"
#include "zlib.h"
#include <sys/types.h>
#include <sys/stat.h>

bool isDir(string pathname)
{
	struct stat info;

	if (stat(pathname.c_str(), &info) != 0)
		return false;
	else if (info.st_mode & S_IFDIR)  // S_ISDIR() doesn't exist on my windows 
		return true;
	else
		return false;
}

bool isFileExists(const std::string& file) {
	struct stat buf;
	return (stat(file.c_str(), &buf) == 0);
}

std::streampos getFileSize(fstream *file)
{
	std::streampos pos = file->tellg();
	file->seekg(0, SEEK_END);
	std::streampos endPos = file->tellg();
	file->seekg(pos, SEEK_SET);
	return endPos;
}

patch::patch(string _path) :filePath(_path)
{
	int main_offset, drm_offset;
	fstream pfile;
	pfile.open(_path, ios_base::in | ios_base::out | ios_base::binary);
	if (!pfile.is_open())
	{
		cout << "Error Opening patch file\n";
		exit(-1);
	}

	main_offset = 0;
	fileHeader tmpHeader(0, &pfile);
	pfile.close();
	file_header &hdr = tmpHeader.get();
	hdr.printHeader();

	tigerFiles[hdr.fileId] = vector<fstream*>(hdr.NumberOfFiles);

	for (size_t i = 0; i < hdr.NumberOfFiles; i++)
	{
		char pathTail[16] = { 0 };
		std::sprintf(pathTail, ".%03d.tiger$2", i);
		string tigerFileName = regex_replace(filePath, regex("[.]([0-9]*)[.]tiger"), pathTail);
		fstream *tmpFile = new fstream(tigerFileName, ios_base::in | ios_base::out | ios_base::binary);

		if (!tmpFile->is_open())
		{
			cout << "Error Opening patch file\n";
			exit(-1);
		}
		tigerFiles[hdr.fileId][i] = tmpFile;
	}
	header = fileHeader(0, tigerFiles[hdr.fileId][0]);
	//header.get().count
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

void patch::unpackAll(string path)
{
	for (size_t i = 0; i < header.get().count; i++)
	{
		unpack(i, path);
	}
}
void patch::process(int id, string path, bool isPacking)
{
	//load table
	vector<elementHeader> elements = header.it();

	if (id<0 || id >(int)elements.size())
		return;
	element &element = elements[id].get();
	system(("mkdir " + path).c_str());

	uint32_t offset = element.offset & 0xFFFFF800;
	uint32_t base = (element.offset & 0x000007F0) >> 4;
	uint32_t file = (element.offset & 0x0000000F);
	int size = element.size;

	DRMHeader drmHeader(offset, tigerFiles[base][file]);
	DRM_Header &drmHdr = drmHeader.get();
	drmHdr.printHeader();

	vector<unknownHeader2> uh2 = drmHeader.it((drmHdr.SectionCount * sizeof(unknown_header1)) + drmHdr.strlen1 + drmHdr.strlen2);
	string subDir = path + "\\" + to_string(id);
	system(("mkdir " + subDir).c_str());

	for (size_t j = 0; j < uh2.size(); j++)
	{
		string subDir = path + "\\" + to_string(id);
		string cdrmFilePath = subDir + "\\" + to_string(j) + ".cdrm";
		unknown_header2 &uheader2 = uh2[j].get();
		uint32_t offset = uheader2.offset & 0xFFFFF800;
		uint32_t base = (uheader2.offset & 0x000007F0) >> 4;
		uint32_t fileNo = uheader2.offset & 0x0000000F;
		if (base != header.get().fileId)
			continue;
		int size = uheader2.size;

		CDRMHeader head(offset, tigerFiles[base][fileNo]);
		head.get().printHeader();
		int cdrmFileFlag = 0;
		if (isPacking)
			cdrmFileFlag = ios_base::binary | ios_base::in;
		else
			cdrmFileFlag = ios_base::binary | ios_base::out;

		fstream cdrmFile(cdrmFilePath, cdrmFileFlag);
		size_t cdrmFileSize = 0;
		if (cdrmFile.is_open())
		{
			if (!isPacking)
			{
				tigerFiles[base][fileNo]->seekg(offset);
				for (int z = 0; z < size; z += 0x800)
				{
					char d[0x800] = { 0 };
					tigerFiles[base][fileNo]->read(d, 0x800);
					cdrmFile.write(d, 0x800);
				}
			}
			else
			{
				cdrmFileSize = getFileSize(&cdrmFile);
			}
		}

		/*rewind to begening of CDRM*/
		tigerFiles[base][fileNo]->seekg(offset);
		vector<CDRMBlockHeader> bh = head.it();
		subDir += "\\" + to_string(j);
		system(("mkdir " + subDir).c_str());
		for (int bh_index = 0; bh_index < bh.size(); bh_index++)
		{
			string uCdrmFilePath = subDir + "\\" + to_string(bh_index) + ".raw";
			CDRM_BlockHeader &cdrmheader = bh[bh_index].get();
			tigerFiles[base][fileNo]->seekg(bh[bh_index].nativeSize(), SEEK_CUR);
			int rawDataStart = tigerFiles[base][fileNo]->tellg();
			if (isPacking && !isFileExists(uCdrmFilePath))
			{
				cout << "\nFile [" << uCdrmFilePath << "] dosen't exist. Skipping...";
				continue;
			}
			fstream uncompressedCdrm(uCdrmFilePath, ios_base::out | ios_base::binary);
			if (!uncompressedCdrm.is_open())
			{
				cout << "\nUnable to open file [" << uCdrmFilePath << "]";
				continue;
			}
			const int compressed_chunk_size = 4096;
			char compressed_data[compressed_chunk_size] = { 0 };

			switch (cdrmheader.blockType)
			{
			case 1:	//uncompressed
				for (size_t current_bytes = 0; current_bytes < cdrmheader.compressedSize; current_bytes += compressed_chunk_size)
				{
					tigerFiles[base][fileNo]->read(compressed_data, compressed_chunk_size);
					uncompressedCdrm.write(compressed_data, compressed_chunk_size);
				}
				break;
			case 2:	//zlib
			{
				auto_ptr<char> compressed_data(new char[cdrmheader.compressedSize]);
				auto_ptr<char> uncompressed_data(new char[cdrmheader.uncompressedSize]);
				uLongf bytesWritten = cdrmheader.uncompressedSize;
				tigerFiles[base][fileNo]->read(compressed_data.get(), cdrmheader.compressedSize);
				if (uncompress((Bytef*)uncompressed_data.get(), &bytesWritten, (Bytef*)compressed_data.get(), cdrmheader.compressedSize) != Z_OK)
				{
					cout << "\nUncompress failure.";
					exit(-1);
				}
				uncompressedCdrm.write(uncompressed_data.get(), cdrmheader.uncompressedSize);
			}
			break;
			case 3:
				cout << "\nskipped. unknown block type.";
				break;
			default:
				cout << "\nUnknown block type.";
				exit(-1);
			}
			uncompressedCdrm.close();

			fstream raw_file(uCdrmFilePath, ios_base::in | ios_base::out | ios_base::binary);
			if (!raw_file.is_open())
			{
				cout << "\nUnable to open file [" << uCdrmFilePath << "]";
				continue;
			}
			uint32_t header = 0;
			raw_file.read((char*)&header, 4);
			raw_file.seekg(0, SEEK_SET);
			switch (header)
			{
			case 0x39444350:	//PDC9
			{
				size_t pcd9_size = getFileSize(&raw_file);
				size_t pcd9SizeNew = 0;
				auto_ptr<char> pcd9_data(new char[pcd9_size]);	//Better way would be to use memory mapped files.
				raw_file.read(pcd9_data.get(), pcd9_size);
				PCD9_Header* pcd9 = (PCD9_Header*)pcd9_data.get();
				string ddsFilePath = subDir + "\\" + to_string(bh_index) + ".dds";
				if (!isPacking)
				{
					DDS dds(pcd9->type, pcd9->height, pcd9->width, 0, pcd9->mipmap + 1, pcd9->format, (char*)(pcd9_data.get()) + sizeof(PCD9_Header), pcd9_size - sizeof(PCD9_Header));
					dds.serialization(ddsFilePath);
				}
				else
				{
					DDS dds;
					dds.deserialization(ddsFilePath);
					pcd9SizeNew = dds.getDataSize() + sizeof(PCD9_Header);
					if (pcd9SizeNew != pcd9_size)
					{
						std::cout << "\nSize of old and new files dont match. This may corrupt files and render game unplayable.";
						std::cout << "\nTruncating for now";
						//auto_ptr<char> pcd9DataNew = auto_ptr<char>(new char[pcd9SizeNew]);
					}
					memcpy((char*)(pcd9_data.get()) + sizeof(PCD9_Header), dds.getData(), pcd9_size - sizeof(PCD9_Header));
					// write to raw file for debugging purpose.
					raw_file.seekp(0, SEEK_SET);
					raw_file.write(pcd9_data.get(), pcd9_size);
				}
			}
			break;
			default:
				break;
			}
			raw_file.close();


			//	pack CDRM
			if (isPacking)
			{
				tigerFiles[base][fileNo]->seekp(rawDataStart, SEEK_SET);
				uncompressedCdrm.open(uCdrmFilePath, ios_base::in | ios_base::binary);
				if (!uncompressedCdrm.is_open())
				{
					cout << "\nUnable to open file [" << uCdrmFilePath << "]";
					continue;
				}

				switch (cdrmheader.blockType)
				{
					/*case 1:	//uncompressed
						for (size_t current_bytes = 0; current_bytes < cdrmheader.compressedSize; current_bytes += compressed_chunk_size)
						{
							tigerFiles[base][fileNo]->read(compressed_data, compressed_chunk_size);
							uncompressedCdrm.write(compressed_data, compressed_chunk_size);
						}
						break;*/
				case 2:	//zlib
				{
					auto_ptr<char> compressed_data(new char[cdrmheader.compressedSize]);
					auto_ptr<char> uncompressed_data(new char[cdrmheader.uncompressedSize]);
					uLongf bytesWritten = cdrmheader.compressedSize;
					uncompressedCdrm.read(uncompressed_data.get(), cdrmheader.uncompressedSize);
					if (compress((Bytef*)compressed_data.get(), &bytesWritten, (Bytef*)uncompressed_data.get(), cdrmheader.uncompressedSize) != Z_OK)
					{
						cout << "\nUncompress failure.";
						exit(-1);
					}
					tigerFiles[base][fileNo]->write(compressed_data.get(), cdrmheader.compressedSize);
					tigerFiles[base][fileNo]->flush();
				}
				break;
				case 3:
					cout << "\nskipped. unknown block type.";
					break;
				default:
					cout << "\nUnknown block type.";
					exit(-1);
				}
				uncompressedCdrm.close();
			}
		}
	}
}
void patch::unpack(int id, string path)
{
	process(id, path, false);
}

void patch::pack(int id, string path)
{
	process(id, path, true);
}

int patch::findEmptyCDRM(size_t sizeHint, uint32_t &offset, int &file, int &base)
{
	string newFileName = regex_replace(filePath, regex("[.]([0-9]*)[.]tiger"), ".004.tiger$2");
	fstream newFile(newFileName, ios_base::in | ios_base::out | ios_base::binary | ios_base::app);

	if (!newFile.is_open())
	{
		cout << "File open fail [" << strerror(errno) << "]";
		return 1;
	}
	header.get().NumberOfFiles = 5;
	header.save();
	//file = 4;
	base = 0;

	while (true)
	{
		offset = (uint32_t)newFile.tellp();

		if (newFile.peek() == EOF)
		{
			newFile.seekg(offset);
			newFile.seekp(offset);
			for (size_t i = 0; i < sizeHint; i += 0x800)
			{
				char data[0x800] = { 0 };
				newFile.write(data, 0x800);
				//cout << "File open fail [" << strerror(errno) << "]";
				newFile.flush();
			}
			break;
		}
		CDRM_Header cdrmHeader;
		cdrmHeader.load(&newFile);
	}
	newFile.close();
	tigerFiles[base].push_back(new fstream(newFileName, ios_base::in | ios_base::out | ios_base::binary));
	file = tigerFiles[base].size() - 1;
	return 0;
}

int patch::getCDRM(uint32_t offset, CDRMHeader & cdrmHeader)
{
	uint32_t fileno = ((offset) & 0xF);
	uint32_t baseno = ((offset) & 0xF0) >> 4;
	offset = ((offset) & 0xFFFFF800);
	cdrmHeader = CDRMHeader(offset, tigerFiles[baseno][fileno]);
	return 0;
}

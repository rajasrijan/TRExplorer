#include "alldef.h"

TIGER::TIGER(string fileLocation, bool _writeDRM) :writeDRM(_writeDRM), currentMode(UNPACK)
{
	cout << "rajasrijan's tiger decoder.\n\n";
	fstream* mainFile = new fstream(fileLocation, ios_base::in | ios_base::out | ios_base::binary);
	if (!mainFile->is_open())
	{
		cout << "Couldn't open file.\nTry running as Admin\n\n";
		exit(-1);
	}
	basePath = fileLocation.substr(0, fileLocation.find_last_of("\\") + 1);
	tigerFiles.push_back(mainFile);
	mainFile->read((char*)&header, sizeof(file_header));
	printHeader();
	if (header.NumberOfFiles > 1)
	{
		cout << "Loading file parts...\n";
		int end_pos = fileLocation.find_last_of('.') - 1;
		int start_pos = fileLocation.find_last_of('.', end_pos) + 1;
		for (uint32_t i = 1; i < header.NumberOfFiles; i++)
		{
			string location = fileLocation;
			char index[4] = { 0 };
			sprintf_s(index, 4, "%03d", i);
			location.replace(start_pos, end_pos - start_pos + 1, index);
			cout << "\tloading \"" << location.c_str() << "\"...";
			fstream* tmp = new fstream(location, ios_base::in | ios_base::out | ios_base::binary);
			if (!tmp->is_open())
			{
				cout << "Failed";
				exit(-1);
			}
			cout << "success\n";
			tigerFiles.push_back(tmp);
		}
	}

	cout << "Loading data entries...";
	elements.resize(header.count);
	mainFile->read((char*)elements.data(), elements.size()*sizeof(element));
	cout << "success\n";
}

TIGER::~TIGER(void)
{
	for (uint32_t i = 0; i < tigerFiles.size(); i++)
		delete tigerFiles[i];
}

void TIGER::printHeader()
{
	cout << "Tiger Header\n";
	cout << "Magic\t\t:" << hex << header.magic << dec;
	cout << "\nVersion\t\t:" << header.version;
	cout << "\nParts\t\t:" << header.NumberOfFiles;
	cout << "\nDRMs\t\t:" << header.count;
	cout << "\nBase Path\t:" << header.BasePath;
	cout << endl;
}

auto_ptr<char> TIGER::getData(int id, uint32_t &size)
{
	uint32_t offset = elements[id].offset & 0xFFFFFF00;
	uint32_t file_no = elements[id].offset & 0x000000FF;
	auto_ptr<char> data(new char[elements[id].size]);
	tigerFiles[file_no]->seekg(offset);
	tigerFiles[file_no]->read(data.get(), elements[id].size);
	size = elements[id].size;
	return data;
}

auto_ptr<char> TIGER::getDecodedData(int id, uint32_t &size, string path)
{
	uint32_t offset = elements[id].offset & 0xFFFFFF00;
	uint32_t file_no = elements[id].offset & 0x000000FF;

	tigerFiles[file_no]->seekg(offset);
	tigerFiles[file_no]->seekp(offset);

	char namehash[9];
	sprintf_s(namehash, 9, "%08x", elements[id].nameHash);
	string filename(namehash);
	string cmd = "mkdir ";
	cmd += path + "\\" + filename;
	int ret = system(cmd.c_str());

	size = elements[id].size;
	return auto_ptr<char>(0);
}

void TIGER::dumpRAW(iostream *dataStream, uint32_t &size, string &path, string &name)
{
	string fullpath = path + "\\" + name;
	ofstream dumpFile(fullpath, ios_base::binary);
	cout << "Writing \"" << fullpath << "\"\n";
	char tmp;
	for (uint32_t i = 0; i < size; i++)
	{
		dataStream->read(&tmp, 1);
		dumpFile.write(&tmp, 1);
	}
	dumpFile.close();
}

uint32_t TIGER::getEntryCount()
{
	return elements.size();
}

TIGER::DataType TIGER::getType(iostream *dataStream)
{
	uint32_t magic = 0;
	streamoff g = dataStream->tellg();
	streamoff p = dataStream->tellp();
	dataStream->read((char*)&magic, sizeof(uint32_t));
	dataStream->seekg(g);
	dataStream->seekp(p);
	return (TIGER::DataType)magic;
}

bool TIGER::decodeDRM(iostream *dataStream, uint32_t &size, string &path, string &name)
{
	DRM_Header table;
	dataStream->read((char*)&table, sizeof(DRM_Header));
	if (table.version != 0x16)
	{
		name = to_string(table.version);

		dataStream->seekg((uint32_t)dataStream->tellg() - sizeof(DRM_Header));
		shared_ptr<char> data(new char[size]);
		memset(data.get(), 0, size);


		dataStream->read(data.get(), size);
		string fullpath = path + "\\" + name + ".DRM";
		cout << "Writing \"" << fullpath << "\"\n";

		fstream output(fullpath, ios_base::binary | ios_base::out);
		if (!output.is_open())
			exit(errno);
		output.write(data.get(), size);
		output.close();
		return false;
	}
	vector<unknown_header1> subentry1(table.SectionCount);
	vector<unknown_header2> subentry2(table.SectionCount);
	vector<char> str(table.strlen1 + table.strlen2);

	if (table.SectionCount == 0)
	{
		size = 0;
		return false;
	}

	streampos subentry2_pos = 0;
	dataStream->read((char*)subentry1.data(), table.SectionCount*sizeof(unknown_header1));
	if ((table.strlen1 + table.strlen2) > 0)
		dataStream->read(str.data(), (table.strlen1 + table.strlen2));
	subentry2_pos = dataStream->tellg();
	dataStream->read((char*)subentry2.data(), table.SectionCount*sizeof(unknown_header2));
	for (uint32_t i = 0; i < table.SectionCount; i++)
	{
		uint32_t offset = subentry2[i].offset & 0xFFFFFF00;;
		uint32_t file_no = subentry2[i].offset & 0x000000FF;;

		tigerFiles[file_no]->seekg(offset);
		tigerFiles[file_no]->seekp(offset);
		char number[10];
		sprintf_s(number, "%d", i);
		uint32_t new_offset = decodeCDRM(tigerFiles[file_no], size, path, name + number);
		if ((new_offset != -1) && (currentMode == PACK))
			subentry2[i].offset = new_offset;
	}
	if ((currentMode == PACK))
	{
		dataStream->seekp(subentry2_pos);
		dataStream->write((char*)subentry2.data(), table.SectionCount*sizeof(unknown_header2));
	}
	return false;
}

uint32_t TIGER::decodeCDRM(iostream *dataStream, uint32_t &size, string &path, string &name)
{
	bool changes = false;
	CDRM_Header table;
	dataStream->read((char*)&table, sizeof(CDRM_Header));
	if (table.magic != 0x4D524443)
		exit(-1);
	vector<CDRM_BlockHeader> BlockHeader(table.count);
	dataStream->read((char*)BlockHeader.data(), table.count*sizeof(CDRM_BlockHeader));

	size = 0;
	uint32_t total_size = 0;
	uint32_t offset = 0;
	for (uint32_t i = 0; i < BlockHeader.size(); i++)	//Calculate total size.
	{
		size += BlockHeader[i].uncompressedSize;
	}
	total_size = size;
	auto_ptr<char> uncompressedData(new char[size]);
	for (uint32_t i = 0; i < BlockHeader.size(); i++)
	{
		uint32_t pos = dataStream->tellg();
		pos = ((pos + 15) / 16) * 16;
		dataStream->seekg(pos);	//Data is 16byte aligned.
		dataStream->seekp(pos);	//Data is 16byte aligned.
		if (BlockHeader[i].blockType == 1)
		{
			dataStream->read(uncompressedData.get() + offset, BlockHeader[i].uncompressedSize);
			offset += BlockHeader[i].uncompressedSize;
		}
		else if (BlockHeader[i].blockType == 2)
		{
			auto_ptr<char> compressedData(new char[BlockHeader[i].compressedSize]);
			dataStream->read(compressedData.get(), BlockHeader[i].compressedSize);
			int ret = Z_OK;
			uLong uncompressSize = size - offset;
			uint8_t *dataPtr = (uint8_t*)uncompressedData.get() + offset;
			ret = uncompress(dataPtr, &uncompressSize, (Bytef*)compressedData.get(), BlockHeader[i].compressedSize);
			offset += BlockHeader[i].uncompressedSize;
			if ((ret != Z_OK) && (ret != Z_STREAM_END))
			{
				exit(ret);
			}
		}
	}
	CDRM_BlockFooter footer;
	dataStream->seekg(((((uint64_t)dataStream->tellg()) + 15) / 16) * 16);	//Data is 16byte aligned.
	dataStream->read((char*)&footer, sizeof(footer));
	switch (((uint32_t*)(uncompressedData.get()))[0])
	{
	case '9DCP':	//PCD9
	{
		uncompressedData = decodePCD9(uncompressedData, size, path, name);
		changes = true;
	}
	break;
	case 0x00000006:
	{
		if ((currentMode == UNPACK))
		{
			try
			{
				string fullpath = path + "\\" + name + ".mesh";
				cout << "Writing \"" << fullpath << "\"\n";
				//Scene scene(uncompressedData.get() , size);
				fstream output(fullpath, ios_base::binary | ios_base::out);
				if (!output.is_open())
					exit(errno);
				output.write(uncompressedData.get(), size);
				output.close();
			}
			catch (exception e)
			{
				cout << e.what();
			}
		}
		//return -1;
	}
	default:
	{
		if (writeDRM && (currentMode == UNPACK))
		{
			string fullpath = path + "\\" + name + ".drm";
			cout << "Writing \"" << fullpath << "\"\n";
			fstream output(fullpath, ios_base::binary | ios_base::out);
			if (!output.is_open())
				exit(errno);
			output.write(uncompressedData.get(), size);
			output.close();
		}
	}
	}
	if (currentMode == PACK /*&& changes*/)
	{
		if (total_size != size)
		{
			cout << "Incorrect size\n";
			exit(-1);
		}

		/*Find next free CDRM*/
		tigerFiles[4]->seekg(0);
		tigerFiles[4]->seekp(0);
		uint32_t blockheaderpos = 0;
		CDRM_Header i;
		while (true)
		{
			uint32_t size = 0;
			i.load(tigerFiles[4]);
			if (i.magic == 0)
				break;
			size += i.count*sizeof(CDRM_BlockHeader);
			size = ((size + 15) / 16) * 16;
			CDRM_BlockHeader j;
			for (int k = 0;k < i.count;k++)
			{
				j.load(tigerFiles[4]);
				size += j.compressedSize;
			}
			size = (((size + 0x800 - 1) / 0x800) * 0x800);
			blockheaderpos += size;
			size -= sizeof(CDRM_Header);
			tigerFiles[4]->seekg(size, ios_base::cur);
			tigerFiles[4]->seekp(size, ios_base::cur);
		}

		tigerFiles[4]->seekg(blockheaderpos);
		tigerFiles[4]->seekp(blockheaderpos);
		tigerFiles[4]->write((char*)&table, sizeof(CDRM_Header));
		blockheaderpos = tigerFiles[4]->tellp();
		tigerFiles[4]->write((char*)BlockHeader.data(), sizeof(CDRM_BlockHeader)*BlockHeader.size());
		tigerFiles[4]->seekp(((((uint64_t)tigerFiles[4]->tellp()) + 15) / 16) * 16);	//Data is 16byte aligned.
		for (uint32_t i = 0; i < BlockHeader.size(); i++)
		{
			BlockHeader[i].uncompressedSize = size;
			if (BlockHeader[i].blockType == 1)
			{
				BlockHeader[i].compressedSize = BlockHeader[i].uncompressedSize;
				tigerFiles[4]->write(uncompressedData.get(), BlockHeader[i].uncompressedSize);
			}
			else if (BlockHeader[i].blockType == 2)
			{
				auto_ptr<char> compressedData(new char[BlockHeader[i].uncompressedSize]);
				int ret = Z_OK;
				BlockHeader[i].compressedSize = BlockHeader[i].uncompressedSize;
				ret = compress2((Bytef*)compressedData.get(), (uLongf*)(&BlockHeader[i].compressedSize), (Bytef*)uncompressedData.get(), BlockHeader[i].uncompressedSize, 4);
				if ((ret != Z_OK) && (ret != Z_STREAM_END))
				{
					cout << "Error Compressing\n";
					exit(ret);
				}
				tigerFiles[4]->write(compressedData.get(), BlockHeader[i].compressedSize);
			}
		}

		uint32_t cdrm_footer = tigerFiles[4]->tellp();
		cdrm_footer = ((cdrm_footer + 15) / 16) * 16;

		footer.relative_offset = 0x800 - (cdrm_footer % 0x800);
		tigerFiles[4]->seekp(cdrm_footer);	//Data is 16byte aligned.
		tigerFiles[4]->write((char*)&footer, sizeof(CDRM_BlockFooter));
		tigerFiles[4]->seekp(blockheaderpos);
		tigerFiles[4]->write((char*)BlockHeader.data(), sizeof(CDRM_BlockHeader)*BlockHeader.size());
		tigerFiles[4]->flush();
		return (blockheaderpos & 0xFFFFFF00) | 4;
	}

	return -1;
}

auto_ptr<char> TIGER::decodePCD9(auto_ptr<char> dataStream, uint32_t &size, string &path, string &name)
{
	PCD9_Header *table = (PCD9_Header*)dataStream.get();
	switch (table->format)
	{
	case '1TXD':	//DXT1
	case '3TXD':	//DXT3
	case '5TXD':	//DXT5
	{
		string fullpath = path + "\\" + name + ".dds";
		if ((currentMode == UNPACK) && (writeDDS))
		{
			DDS dds(table->type, table->height, table->width, 0, table->mipmap + 1, table->format, (char*)dataStream.get() + sizeof(PCD9_Header), size - sizeof(PCD9_Header));
			cout << "Writing \"" << fullpath << "\"\n";
			dds.serialization(path + "\\" + name + ".dds");
			size = 0;
		}
		else if (currentMode == PACK)
		{
			DDS dds;
			if (!dds.deserialization(fullpath))
			{
				cout << "\nError opening dds file.\n";
				exit(-1);
			}
			PCD9_Header tmp = *table;
			size = dds.dataSize + sizeof(PCD9_Header);
			dataStream.reset(new char[size]);
			table = (PCD9_Header*)dataStream.get();
			memcpy((char*)table + sizeof(PCD9_Header), dds.data, dds.dataSize);
			*table = tmp;
		}
		return dataStream;
	}
	break;
	case 0x15:
	{
		string fullpath = path + "\\" + name + ".raw";
		if (currentMode == UNPACK)
		{
			cout << "Writing \"" << fullpath << "\"\n";
			fstream rawFile(fullpath, ios_base::binary | ios_base::out);
			if (!rawFile.is_open())
				exit(errno);
			rawFile.write((char*)dataStream.get() + sizeof(PCD9_Header), size - sizeof(PCD9_Header));
			rawFile.close();
		}
		size = 0;
		return dataStream;
	}
	break;
	default:
		break;
	}
	cout << "Something went wrong." << __FILE__ << "," << __FUNCTIONW__ << "," << __LINE__ << "\n";
	return dataStream;
}

fstream* TIGER::getRepositionedStream(uint32_t offset)
{
	uint32_t off = offset & 0x000000FF;
	uint32_t file_no = offset & 0xFFFFFF00;

	tigerFiles[file_no]->seekg(off);
	tigerFiles[file_no]->seekp(off);

	return tigerFiles[file_no];
}

bool TIGER::unpack(uint32_t id, string path)
{
	//	Find and replace.
	uint32_t offset = elements[id].offset & 0xFFFFFF00;
	uint32_t file_no = elements[id].offset & 0x000000FF;

	tigerFiles[file_no]->seekg(offset);
	tigerFiles[file_no]->seekp(offset);

	char namehash[9];
	sprintf_s(namehash, 9, "%08x", elements[id].nameHash);
	string filename(namehash);
	path = path + "\\" + filename;
	string cmd = "mkdir " + path;
	int ret = system(cmd.c_str());

	writeDRM = true;
	writeDDS = true;
	currentMode = UNPACK;
	uint32_t size = elements[id].size;;
	ret = decodeDRM(tigerFiles[file_no], size, path, string(""));
	return true;
}

bool TIGER::pack(uint32_t nameHash, string path)
{
	//	Find the element with hash.
	uint32_t id = findByHash(nameHash);

	//	Create new file if needed.
	if (tigerFiles.size() < 5)
	{
		ofstream newFile(basePath + "bigfile.004.tiger");
		if (newFile.is_open())
		{
			for (int i = 0; i < 0x800 * 1024; i++)
			{
				newFile.write("\x00", 1);
			}
			header.NumberOfFiles = 5;
			tigerFiles[0]->seekg(0);
			tigerFiles[0]->seekp(0);
			tigerFiles[0]->write((char*)&header, sizeof(header));
			tigerFiles[0]->flush();
			newFile.close();
			tigerFiles.push_back(new fstream(basePath + "bigfile.004.tiger", ios_base::in | ios_base::out | ios_base::binary));
		}
		else
		{
			return false;
		}
	}

	//	Create history file if necessary
	fstream history(basePath + "bigfile.tiger.history", ios_base::app);

	//	Find and replace.
	uint32_t offset = elements[id].offset & 0xFFFFFF00;
	uint32_t file_no = elements[id].offset & 0x000000FF;

	tigerFiles[file_no]->seekg(offset);
	tigerFiles[file_no]->seekp(offset);

	char namehash[9];
	sprintf_s(namehash, 9, "%08x", elements[id].nameHash);
	string filename(namehash);
	path = path + "\\" + filename;
	//int ret = system(cmd.c_str());

	//	DRM
	currentMode = PACK;
	uint32_t size = elements[id].size;
	bool ret = decodeDRM(tigerFiles[file_no], size, path, string(""));
	return true;
}

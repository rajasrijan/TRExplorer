#define _CRT_SECURE_NO_WARNINGS
#include "patch.h"
#include <algorithm>
#include <regex>
#include <assert.h>
#include "alldef.h"
#include "zlib.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>
#include "Poco/DOM/AutoPtr.h"
#include "Poco/DOM/DOMWriter.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/Element.h"
#include "Poco/DOM/Text.h"
#include "Poco/XML/XMLWriter.h"
#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/NodeIterator.h"
#include "Poco/DOM/NodeFilter.h"
#include "Poco/SAX/InputSource.h"
#include "Poco/DOM/NamedNodeMap.h"
#include "Poco/DOM/NodeList.h"
#include "Poco/Exception.h"

using Poco::XML::Document;
using Poco::XML::Element;
using Poco::XML::Text;
using Poco::XML::AutoPtr;
using Poco::XML::DOMWriter;
using Poco::XML::XMLWriter;
using Poco::XML::DOMParser;
using Poco::XML::InputSource;
using Poco::XML::NodeIterator;
using Poco::XML::NodeFilter;
using Poco::XML::Node;
using Poco::XML::NodeList;
using Poco::XML::NamedNodeMap;
using Poco::Exception;

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

int makeDirectory(string path, bool recursive = false)
{
	if (!recursive && !isDir(path))
		return _mkdir(path.c_str());
	else
	{
		return 0;
	}
	return 0;
}

bool isFileExists(const std::string& file) {
	struct stat buf;
	return (stat(file.c_str(), &buf) == 0);
}

bool dir(vector<string> &resultList, string pattern = "")
{
	string command = "dir /B \"" + pattern + "\"";
	FILE* dirStdout = _popen(command.c_str(), "r");
	char buffer[1024];
	char* line = nullptr;
	while ((line = fgets(buffer, sizeof(buffer), dirStdout)) != nullptr)
	{
		size_t pathLength = strlen(line);
		if (pathLength > 0 && line[pathLength - 1] == '\n')
			line[pathLength - 1] = 0;
		resultList.push_back(line);
	}
	_pclose(dirStdout);
	return 0;
}

string pathJoin(string first, string second)
{
	bool fe = (*first.rbegin()) == '\\';
	bool ss = (*second.begin()) == '\\';
	if (fe == false && ss == false)
	{
		return first + "\\" + second;
	}
	else if ((fe == true && ss == false) || (fe == false && ss == true))
	{
		return first + second;
	}
	else if ((fe == true && ss == true))
	{
		return first + second.substr(1, second.length() - 1);
	}
	return "";
}

std::streampos getFileSize(fstream *file)
{
	std::streampos pos = file->tellg();
	file->seekg(0, SEEK_END);
	std::streampos endPos = file->tellg();
	file->seekg(pos, SEEK_SET);
	return endPos;
}

int getXmlNodeAttribute(const Node* pNode, const string &attrName, uint32_t &attrVal)
{
	if (!pNode)
		return 1;
	AutoPtr<NamedNodeMap> attributes = pNode->attributes();
	if (attributes.isNull())
		return 1;
	Node *attrNode = attributes->getNamedItem(attrName);
	if (!attrNode)
		return 1;
	attrVal = atoi(attrNode->getNodeValue().c_str());
	return 0;
}

int getXmlNodeValue(const Node* pNode, string &val)
{
	if (!pNode)
		return 1;
	val = pNode->getNodeValue();
	return 0;
}

patch::patch(string _tigerPath, bool silent) :tigerFilePath(_tigerPath)
{
	int main_offset;
	fstream pfile;
	pfile.open(tigerFilePath, ios_base::in | ios_base::out | ios_base::binary);
	if (!pfile.is_open())
	{
		cout << "Error Opening patch file\n";
		exit(1);
	}

	main_offset = 0;
	fileHeader tmpHeader(0, &pfile);
	pfile.close();
	file_header &hdr = tmpHeader.get();
	if (!silent)
		hdr.printHeader();

	tigerFiles[hdr.fileId] = vector<fstream*>(hdr.NumberOfFiles);

	for (size_t i = 0; i < hdr.NumberOfFiles; i++)
	{
		char pathTail[16] = { 0 };
		std::sprintf(pathTail, ".%03d.tiger$2", i);
		string tigerFileName = regex_replace(tigerFilePath, regex("[.]([0-9]*)[.]tiger"), pathTail);
		fstream *tmpFile = new fstream(tigerFileName, ios_base::in | ios_base::out | ios_base::binary);

		if (!tmpFile->is_open())
		{
			cout << "Error Opening patch file\n";
			exit(1);
		}
		tigerFiles[hdr.fileId][i] = tmpFile;
	}
	header = fileHeader(0, tigerFiles[hdr.fileId][0]);
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

int patch::process(int id, string path, bool isPacking, bool silent)
{
	//load table
	int errorCode = 0;
	uint32_t hdr = 0;
	vector<elementHeader> elements = header.getIterator();

	if (id<0 || id >(int)elements.size())
		return 1;
	element &element = elements[id].get();
	system(("mkdir " + path).c_str());

	uint32_t offset = element.offset & 0xFFFFF800;
	uint32_t base = (element.offset & 0x000007F0) >> 4;
	uint32_t file = (element.offset & 0x0000000F);
	tigerFiles[base][file]->seekg(offset, SEEK_SET);
	tigerFiles[base][file]->seekp(offset, SEEK_SET);
	int size = element.size;
	hdr = 0;
	tigerFiles[base][file]->read((char*)&hdr, 4);
	tigerFiles[base][file]->seekg(-4, SEEK_CUR);
	switch (hdr)
	{
	case 0:
	case 1:
	case 2:
	{
		cout << "LOCAL" << endl;
		string subDir = path + "\\" + to_string(id);
		string localFilePath = path + "\\" + to_string(id) + ".local";

		//errorCode = makeDirectory(subDir);
		//if (errorCode)exit(1);

		fstream localDump(localFilePath, ios_base::binary | ios_base::out);
		char tmpData[0x800] = { 0 };
		for (uint32_t i = 0; i < element.size; i += sizeof(tmpData))
		{
			tigerFiles[base][file]->read(tmpData, sizeof(tmpData));
			localDump.write(tmpData, sizeof(tmpData));
		}
		localDump.close();
		fstream textFileObject(localFilePath + ".xml", isPacking ? ios_base::in : ios_base::out);
		if (!isPacking)
		{
			AutoPtr<Document> pDoc = new Document;
			localDump.open(localFilePath, ios_base::binary | ios_base::in);
			StringTableHeader localTable(0, &localDump);

			vector<uint32_t> indexList(localTable.get().unknownCount);
			localDump.seekg(localTable.nativeSize(), SEEK_SET);
			localDump.read((char*)indexList.data(), localTable.get().unknownCount * sizeof(uint32_t));

			vector<StringTableOffset> offsetList = localTable.getIterator(localTable.get().unknownCount * sizeof(uint32_t));
			AutoPtr<Element> pRoot = pDoc->createElement("StringTable");
			pRoot->setAttribute("local", to_string(localTable.get().local));
			pRoot->setAttribute("unknownCount", to_string(localTable.get().unknownCount));
			pRoot->setAttribute("tableRange", to_string(localTable.get().tableRange));
			pDoc->appendChild(pRoot);
			AutoPtr<Element> pTestListNode = pDoc->createElement("TextList");
			pRoot->appendChild(pTestListNode);
			AutoPtr<Element> pIndexListNode = pDoc->createElement("IndexList");
			pRoot->appendChild(pIndexListNode);
			for (auto i : indexList)
			{
				AutoPtr<Element> pChild1 = pDoc->createElement("index");
				pChild1->setAttribute("fileOffset", to_string(i));
				pIndexListNode->appendChild(pChild1);
			}
			for (auto i : offsetList)
			{
				char c = 0;
				stringstream localText;
				if (i.get())
				{
					localDump.seekg(i.get(), SEEK_SET);
					while ((c = localDump.get()) != 0)
					{
						localText << c;
					}
				}
				AutoPtr<Element> pChild1 = pDoc->createElement("text");
				pChild1->setAttribute("fileOffset", to_string(i.get()));
				AutoPtr<Text> pText1 = pDoc->createTextNode(localText.str());
				pChild1->appendChild(pText1);
				pTestListNode->appendChild(pChild1);
			}
			DOMWriter writer;
			writer.setNewLine("\n");
			writer.setOptions(XMLWriter::PRETTY_PRINT);
			writer.writeNode(textFileObject, pDoc);
			localDump.close();
		}
		else
		{
			InputSource src(textFileObject);
			DOMParser parser;
			AutoPtr<Document> pDoc = parser.parse(&src);
			localDump.open(localFilePath, ios_base::binary | ios_base::out);
			StringTable_Header st;
			Node *pRootNode = pDoc->firstChild();
			if (pRootNode->nodeName() != "StringTable")
			{
				cout << "ERROR: Invalid StringTable xml.";
				exit(1);
			}
			if (getXmlNodeAttribute(pRootNode, "local", st.local)
				|| getXmlNodeAttribute(pRootNode, "tableRange", st.tableRange)
				|| getXmlNodeAttribute(pRootNode, "unknownCount", st.unknownCount))
			{
				cout << "ERROR: Invalid StringTable xml.";
				exit(1);
			}
			StringTableHeader(0, &localDump, st).save();
			for (uint32_t i = 0; i < st.unknownCount; i++)
			{
				uint32_t pad = i + 1;
				localDump.write((char*)&pad, sizeof(pad));
			}
			vector<pair<uint32_t, string>> locals;
			for (Node* textNode = pRootNode->firstChild(); textNode != nullptr; textNode = textNode->nextSibling())
			{
				if (textNode->nodeType() != Node::ELEMENT_NODE)
					continue;
				pair<uint32_t, string> local;
				getXmlNodeAttribute(textNode, "fileOffset", local.first);
				getXmlNodeValue(textNode->firstChild(), local.second);
				locals.push_back(local);
				localDump.write((char*)&local.first, sizeof(local.first));
			}

			// now write all the text.
			for (auto local : locals)
			{
				uint32_t textOffset = local.first;
				if (textOffset == 0)
				{
					continue;
				}
				uint32_t currentFileOffset = localDump.tellp();
				for (size_t i = 0; i < textOffset - currentFileOffset; i++)
				{
					char pad = 0;
					localDump.write(&pad, 1);
				}
				localDump.seekp(textOffset, SEEK_SET);

				localDump.write(local.second.c_str(), local.second.size());
			}
			// zero pad till 0x800 aligned
			uint32_t currentFileOffset = localDump.tellp();
			uint32_t endOffset = ((currentFileOffset + 0x800 - 1) / 0x800) * 0x800;
			for (size_t i = 0; i < endOffset - currentFileOffset; i++)
			{
				char pad = 0;
				localDump.write(&pad, 1);
			}
			localDump.close();
		}
	}
	break;
	case 0x16:
	{
		cout << "DRM";
		DRMHeader drmHeader(offset, tigerFiles[base][file]);
		DRM_Header &drmHdr = drmHeader.get();
		if (!silent)drmHdr.printHeader();

		vector<unknownHeader2> uh2 = drmHeader.getIterator((drmHdr.SectionCount * sizeof(unknown_header1)) + drmHdr.strlen1 + drmHdr.strlen2);
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
			if (!silent)head.get().printHeader();
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
			vector<CDRMBlockHeader> bh = head.getIterator();
			subDir += "\\" + to_string(j);
			system(("mkdir " + subDir).c_str());
			int cdrmBlockHeaderOffset = (int)tigerFiles[base][fileNo]->tellg();
			int lastCdrmBlockSize = 0;
			cdrmBlockHeaderOffset = ((cdrmBlockHeaderOffset + 0xF) / 0x10) * 0x10;
			for (int bh_index = 0; bh_index < bh.size(); bh_index++)
			{
				cdrmBlockHeaderOffset += lastCdrmBlockSize;
				string uCdrmFilePath = subDir + "\\" + to_string(bh_index) + ".dmp";
				CDRM_BlockHeader &cdrmheader = bh[bh_index].get();
				lastCdrmBlockSize = (((cdrmheader.compressedSize + 0x0F) / 0x10) * 0x10);
				tigerFiles[base][fileNo]->seekg(cdrmBlockHeaderOffset, SEEK_SET);
				int rawDataStart = tigerFiles[base][fileNo]->tellg();
				if (isPacking && !isFileExists(uCdrmFilePath))
				{
					cout << "\nError: File [" << uCdrmFilePath << "] dosen't exist. Skipping...";
					continue;
				}
				fstream uncompressedCdrm(uCdrmFilePath, ios_base::out | ios_base::binary);
				if (!uncompressedCdrm.is_open())
				{
					cout << "\nError: Unable to open file [" << uCdrmFilePath << "]";
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
					int lok = tigerFiles[base][fileNo]->tellg();
					tigerFiles[base][fileNo]->read(compressed_data.get(), cdrmheader.compressedSize);
					if ((errorCode = uncompress((Bytef*)uncompressed_data.get(), &bytesWritten, (Bytef*)compressed_data.get(), cdrmheader.compressedSize)) != Z_OK)
					{
						cout << "\nError: Uncompress failure. Error code:" << errorCode;
						exit(1);
					}
					uncompressedCdrm.write(uncompressed_data.get(), cdrmheader.uncompressedSize);
				}
				break;
				case 3:
					cout << "\nError: skipped. unknown block type.";
					break;
				default:
					cout << "\nError: Unknown block type.";
					exit(1);
				}
				uncompressedCdrm.close();

				fstream raw_file(uCdrmFilePath, ios_base::in | ios_base::out | ios_base::binary);
				if (!raw_file.is_open())
				{
					cout << "\nError: Unable to open file [" << uCdrmFilePath << "]";
					continue;
				}
				hdr = 0;
				raw_file.read((char*)&hdr, 4);
				raw_file.seekg(0, SEEK_SET);
				switch (hdr)
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
						cout << ddsFilePath << endl;
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
						cout << "\nError: Unable to open file [" << uCdrmFilePath << "]";
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
							cout << "\nError: Uncompress failure.";
							exit(1);
						}
						tigerFiles[base][fileNo]->write(compressed_data.get(), cdrmheader.compressedSize);
						tigerFiles[base][fileNo]->flush();
					}
					break;
					case 3:
						cout << "\nError: skipped. unknown block type.";
						break;
					default:
						cout << "\nError: Unknown block type.";
						exit(1);
					}
					uncompressedCdrm.close();
				}
			}
		}
	}
	break;
	case 0xAC44:	// audio format of some sort.
	{
		cout << "AUDIO" << endl;
		string subDir = path + "\\" + to_string(id);
		string audioFilePath = path + "\\" + to_string(id) + ".audio";

		//errorCode = makeDirectory(subDir);
		//if (errorCode)exit(1);

		fstream localDump;
		if (!isPacking)
		{
			localDump.open(audioFilePath, ios_base::binary | ios_base::out);
			char tmpData[16] = { 0x0 };
			for (size_t i = 0; i < size; i += sizeof(tmpData))
			{
				tigerFiles[base][file]->read(tmpData, sizeof(tmpData));
				localDump.write(tmpData, sizeof(tmpData));
			}
		}
		else
		{
			localDump.open(audioFilePath, ios_base::binary | ios_base::in);
			char tmpData[16] = { 0x0 };
			for (size_t i = 0; i < size; i += sizeof(tmpData))
			{
				localDump.read(tmpData, sizeof(tmpData));
				tigerFiles[base][file]->write(tmpData, sizeof(tmpData));
			}
		}
	}
	break;
	default:
		errorCode = 1;
		break;
	}
	return errorCode;
}

int patch::unpack(int id, string path, bool silent)
{
	return process(id, path, false, silent);
}

void patch::pack(int id, string path)
{
	process(id, path, true);
}

int patch::findEmptyCDRM(size_t sizeHint, uint32_t &offset, int &file, int &base)
{
	string newFileName = regex_replace(tigerFilePath, regex("[.]([0-9]*)[.]tiger"), ".004.tiger$2");
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

void patch::printNameHashes()
{
	vector<elementHeader> elements = header.getIterator();
	for (elementHeader it : elements)
	{
		cout << hex << it.get().nameHash << "\n";
	}
}
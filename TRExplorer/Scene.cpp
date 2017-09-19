/*
MIT License

Copyright (c) 2017 Srijan Kumar Sharma

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include "Scene.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <DirectXPackedVector.h>

using namespace DirectX;

Scene::Scene(void* data_ptr, size_t sz, const string &path)
{
	//	Check correct format or not.
	m_Scene = new(data_ptr) Scene_header;
	if (!m_Scene->IsValid())
	{
		throw("Incorrect format.%s,%d", __FILE__, __LINE__);
	}

	m_MeshHeader = new(m_Scene->getMeshHeaderPtr()) SceneHeader;
	if (!m_MeshHeader->IsValid())
	{
		throw("Incorrect format.%s,%d", __FILE__, __LINE__);
	}

	//	Load model.
	//	Pointer to start of mesh structures.
	//char* MESH_START = ((char*)m_MeshHeader);

	//	Load bones.
	if (m_MeshHeader->getBoneCount() > 0)
	{
		/*uint32_t PTR_BONE_COUNT = m_Scene->bone_ptr[0] - 4;
		uint32_t PTR_BONE_START = m_Scene->bone_ptr[1];
		uint32_t PTR_UNKN_COUNT = m_Scene->bone_ptr[2] - 4;
		uint32_t PTR_UNKN_START = m_Scene->bone_ptr[3];*/
	}
	int faceGroupIndex = 0;
	//	Load mesh.
	Mesh *m_Mesh = reinterpret_cast<Mesh*>(m_MeshHeader->getMeshPtr());
	for (int matrixIndex = 0; matrixIndex < m_MeshHeader->getMeshCount(); matrixIndex++)
	{
		std::fstream meshFile(path + "_" + to_string(matrixIndex) + ".obj", std::ios_base::out);
		if (!meshFile.is_open())
		{
			exit(1);
		}

		//	Parse Bone Map
		if (m_MeshHeader->getBoneCount() > 0)
		{

		}

		//	Parse Vertex Format
		/*	Skip to FVF Table*/
		MeshItemList *mil = (MeshItemList*)((char*)m_MeshHeader + m_Mesh[matrixIndex].vfOffset);
		MeshItem *meshItem = mil->getMeshItemPtr();

		uint16_t mesh_xyz = 0, uv_pos = 0, uv2_pos = 0, bw_pos = 0, bi_pos = 0, norm_pos = 0, tmp = 0;
		for (int j = 0; j < mil->count; j++)
		{
			switch (meshItem[j].itemOffset)
			{
			case 0xd2f7d823:	//	0 Position
				mesh_xyz = meshItem[j].itemValue;
				break;
			case 0x8317902A:	//	#19 TexCoord
				uv_pos = meshItem[j].itemValue;
				break;
			case 0x8E54B6F3:	//	#19 TexCoord
				uv2_pos = meshItem[j].itemValue;
				break;
			case 0x48e691c0:	//	#6 BoneWeight
				bw_pos = meshItem[j].itemValue;
				break;
			case 0x5156d8d3:	//	#7 BoneIndicies
				bi_pos = meshItem[j].itemValue;
				break;
			case 0x3e7f6149:	//	#3 ?? (16)
				continue;	//#mesh.unkn_pos = itemValue
				break;
			case 0x36f5e414:	//	#5 Normals + 0x64a86f01 + 0xf1ed11c3
				norm_pos = meshItem[j].itemValue;
				break;
			default:
				tmp = meshItem[j].itemValue;
				break;
			}
		}
		std::vector<XMFLOAT3> vectorList;

		for (size_t k = 0; k < m_Mesh[matrixIndex].numVerts; k++)
		{
			size_t idx = k*mil->vertexSize;
			float* ptr = (float*)((char*)m_MeshHeader + m_Mesh[matrixIndex].vertex_offset + idx + mesh_xyz);
			meshFile << "v " << ptr[0] << " " << ptr[1] << " " << ptr[2] << "\n";
		}
		meshFile << "\n";

		if (m_MeshHeader->getMeshCount() == 1 && (m_Mesh[matrixIndex].numFaces * 2) == m_Mesh[matrixIndex].numVerts)
		{
			printf("%s : Not supported yet\n", __FUNCTION__);
			continue;
		}
		else
		{
			int faceCount = 0;
			bool loopDone = false;
			unknown_meshheader *umh = (unknown_meshheader*)m_MeshHeader->getMatTablePtr();
			for (size_t faceId = 0; faceId < m_Mesh[matrixIndex].num_faces; faceId++)
			{
				cout << "Material {" << matrixIndex << "} [" << umh[faceGroupIndex].matNum << "]" << endl;
				char* tmp = (char*)m_MeshHeader->getFaceTablePtr();
				uint16_t* idxBuffer = (uint16_t*)tmp + umh[faceGroupIndex].indexFrom;
				for (size_t z = 0; z < umh[faceGroupIndex].numFaes * 3; z += 3)
				{
					meshFile << "f " << idxBuffer[z] + 1 << " " << idxBuffer[z + 1] + 1 << " " << idxBuffer[z + 2] + 1 << "\n";
				}
				meshFile << "\n";
				faceGroupIndex++;
			}
		}
	}
}


Scene::~Scene(void)
{
}

Scene_header::Scene_header()
{
}

Scene_header::~Scene_header()
{
}

bool Scene_header::IsValid()
{
	return true;
}

int Scene_header::getMeshCount()
{
	return no_of_meshes;
}

void * Scene_header::getMeshHeaderPtr()
{
	return ((char*)this) + (headerSize * 8) + 20 + (sizeof(uint32_t)*no_of_meshes) + mesh_offset;
}

SceneHeader::SceneHeader()
{
}

SceneHeader::~SceneHeader()
{
}

bool SceneHeader::IsValid()
{
	return (magic == 'hseM');
}

int SceneHeader::getBoneCount()
{
	return no_bones;
}

int SceneHeader::getMeshCount()
{
	return no_mesh;
}

void * SceneHeader::getMeshPtr()
{
	return (void*)((char*)this + mesh_offset0);
}

int SceneHeader::getFaceGroupsCount()
{
	return m_noFaceGroups;
}

void * SceneHeader::getMatTablePtr()
{
	return (char*)this + mat_tbl;
}

void * SceneHeader::getFaceTablePtr()
{
	return (char*)this + face_tbl;
}

MeshItem * MeshItemList::getMeshItemPtr()
{
	return &firstItem;
}

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

#pragma once
#include <stdint.h>
#include <vector>
#include <string>

using namespace std;

#pragma pack(push,1)

struct Mesh
{
	uint32_t num_faces;
	uint16_t u1;
	uint16_t bone_map_count;
	uint32_t bone_map_offset;
	uint32_t vertex_offset;
	uint32_t u2[3];
	uint32_t vfOffset;
	uint32_t numVerts;
	uint32_t u3;
	uint32_t facesOffset;
	uint32_t numFaces;
};

class SceneHeader
{
	uint32_t magic;
	uint32_t u1[2];
	uint32_t no_tris;
	uint8_t u2[100];
	uint32_t mat_tbl, mesh_offset0, mesh_tbl1, bone_tbl, face_tbl;
	uint16_t m_noFaceGroups, no_mesh, no_bones, no_unk03;
public:
	SceneHeader();
	~SceneHeader();
	bool IsValid();
	int getBoneCount();
	int getMeshCount();
	void* getMeshPtr();
	int getFaceGroupsCount();
	void *getMatTablePtr();
	void *getFaceTablePtr();
};
struct unknown_meshheader
{
	uint32_t u1[4];
	uint32_t indexFrom;
	uint32_t numFaes;
	uint32_t u2[4];
	uint32_t matNum;
	uint32_t u3[9];
};
class Scene_header
{
	uint32_t headerSize;
	uint32_t u1[3];
	uint32_t no_of_meshes;
	uint32_t u2;
	uint32_t mesh_offset;
public:
	Scene_header();
	~Scene_header();
	bool IsValid();
	int getMeshCount();
	void* getMeshHeaderPtr();
};

struct MeshItem
{
	uint32_t itemOffset;
	uint16_t itemValue;
	uint16_t itemType;
};

struct MeshItemList
{
	uint32_t u1, u2;
	uint16_t count;
	uint16_t vertexSize;
	uint8_t u3[4];
	MeshItem firstItem;
	MeshItem* getMeshItemPtr();
};

#pragma pack(pop)
class Scene
{
private:
	Scene_header *m_Scene;
	uint32_t *unknown;
	uint8_t *unknown1;
	SceneHeader *m_MeshHeader;
	std::vector<Mesh> meshList;
public:
	Scene(void* data_ptr, size_t sz, const string &path);
	~Scene(void);
};


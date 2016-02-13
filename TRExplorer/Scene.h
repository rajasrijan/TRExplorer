#pragma once
#include <stdint.h>
#include <vector>

struct Scene_header
{
	uint32_t magic;
	uint32_t u1[3];
	uint32_t no_of_meshes;
	uint32_t u2;
	uint32_t mesh_ptr;
	uint32_t u3[6];
	uint32_t bone_ptr[4];
};

struct Scene_header2
{
	uint32_t magic;
	uint32_t u1[2];
	uint32_t no_tris;
	uint8_t u2[100];
	uint32_t mat_tbl,mesh_tbl0,mesh_tbl1,bone_tbl,face_tbl;
	uint16_t no_fgroups,no_mesh,no_bones,no_unk03;
};

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

class Scene
{
private:
	Scene_header *header;
	uint32_t *unknown;
	uint8_t *unknown1;
	Scene_header2 *header2;
	std::vector<Mesh> meshList;
public:
	Scene(void* data_ptr,size_t sz);
	~Scene(void);
};


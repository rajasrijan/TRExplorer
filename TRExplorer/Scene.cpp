#include "Scene.h"
#include <iostream>

Scene::Scene(void* data_ptr,size_t sz)
{
	//	Check correct format or not.
	header = (Scene_header*)data_ptr;
	if (header->magic!=0x06)
	{
		throw("Incorrect format.%s,%d",__FILE__,__LINE__);
	}
	unknown = (uint32_t*)(&header[1]);
	unknown1 = (uint8_t*)(&unknown[header->no_of_meshes]);
	header2 = (Scene_header2*)(&unknown1[header->mesh_ptr]);
	if (header2->magic!='hseM')
	{
		throw("Incorrect format.%s,%d",__FILE__,__LINE__);
	}

	//	Load model.

	//	Pointer to start of mesh structures.
	char* MESH_START = ((char*)header2);

	//	Load bones.
	if (header2->no_bones>0)
	{
		uint32_t PTR_BONE_COUNT = header->bone_ptr[0] - 4;
		uint32_t PTR_BONE_START = header->bone_ptr[1];
		uint32_t PTR_UNKN_COUNT = header->bone_ptr[2] - 4;
		uint32_t PTR_UNKN_START = header->bone_ptr[3];
		throw("INCOMPLETE!");
	}

	//	Load mesh.
	for (int i = 0; i < header2->no_mesh; i++)
	{
		Mesh *mesh = &((Mesh*)(MESH_START + header2->mesh_tbl0))[i];
		//	Parse Bone Map
		if (header2->no_bones>0)
		{

		}

		//	Parse Vertex Format
		/*	Skip to FVF Table*/
		uint16_t* itemCount = (uint16_t*)(MESH_START + mesh->vfOffset	+ \
			/*	Skip past unknown 0x8*/
			+ 8);
		uint16_t* vertexSize = itemCount + 1;

		struct vert_fmt
		{
			uint32_t itemOffset;
			uint16_t itemValue;
			uint16_t itemType;
		}*f = (vert_fmt*)(((char*)vertexSize) + 6);
		uint16_t mesh_xyz,uv_pos,uv2_pos,bw_pos,bi_pos,norm_pos;
		for (int j = 0; j < *itemCount; j++)
		{
			switch (f[j].itemOffset)
			{
			case 0xd2f7d823:	//	0 Position
				mesh_xyz=f[i].itemValue;
				break;
			case 0x8317902A:	//	#19 TexCoord
				uv_pos=f[i].itemValue;
				break;
			case 0x8E54B6F3:	//	#19 TexCoord
				uv2_pos = f[i].itemValue;
				break;
			case 0x48e691c0:	//	#6 BoneWeight
				bw_pos = f[i].itemValue;
				break;
			case 0x5156d8d3:	//	#7 BoneIndicies
				bi_pos = f[i].itemValue;
				break;
			case 0x3e7f6149:	//	#3 ?? (16)
				continue;	//#mesh.unkn_pos = itemValue
				break;
			case 0x36f5e414:	//	#5 Normals + 0x64a86f01 + 0xf1ed11c3
				norm_pos = f[i].itemValue;
				break;
			default:
				break;
			}
		}
	}
}


Scene::~Scene(void)
{
}

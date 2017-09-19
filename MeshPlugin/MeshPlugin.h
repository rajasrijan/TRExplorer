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
#define PLUGIN
#include "PluginInterface.h"
#include <DirectXMath.h>
#include <string>
#include "tinyxml2.h"

using namespace std;
using namespace DirectX;
using namespace tinyxml2;

#pragma pack(push,1)
struct FaceHeader
{
    uint32_t unknown[4];
    int32_t indexFrom;
    int32_t numFaces;
    uint32_t unknown2[4];
    int32_t matNum;
    uint32_t unknown3[9];
    size_t getIndexBufferOffset();
};
struct BoneHeader
{
    uint32_t bone_pointers[4];
};
enum ComponentConstant
{
    CMP_Offset_Position = 0xd2f7d823, //0 Position
    CMP_Offset_TexCord = 0x8317902A, //19 TexCoord
    CMP_Offset_TexCord2 = 0x8E54B6F3, //19 TexCoord
    CMP_Offset_BoneWeight = 0x48e691c0, //6 BoneWeight
    CMP_Offset_BoneIndex = 0x5156d8d3, //7 BoneIndicies
    CMP_Offset_Unknown = 0x3e7f6149, //3 ?? (16)
    CMP_Offset_Normal = 0x36f5e414, //5 Normals + 0x64a86f01 + 0xf1ed11c3
};
struct ComponentValue
{
    uint32_t itemOffsetType;
    int16_t itemValue;
    int16_t	itemType;
};
struct ComponentDataBlob
{
    uint32_t unknown[2];								//Skip past unknown 0x8
    uint16_t itemCount;									//itemCount
    uint16_t vertexSize;								//vertexSize
    uint32_t unknown2;
    ComponentValue* getComponents(int componentIdx);
};
struct MeshHeader
{
    int32_t numFaceGroups;
    uint16_t unknown;
    uint16_t BoneMapCount;
    int32_t BoneMapOffset;
    int32_t vertOffset;
    uint32_t unknown2[3];
    int32_t vfOffset;
    int32_t numVerts;
    int32_t unknown3;
    int32_t facesOffset;
    int32_t numFaces;
};
struct SceneHeader
{
    union
    {
        uint32_t magic;
        char magicStr[4];
    };
    //	4 bytes
    uint32_t unknown[2];
    //	12 bytes
    uint32_t NUM_TRIS;
    //	16 bytes
    uint32_t unknown2[25];	// 100 bytes of unknown
    //	116 bytes
    int32_t PTR_MAT_TABLE;
    //	120 bytes
    int32_t PTR_MESH_TBL00;
    int32_t PTR_MESH_TBL01;
    int32_t PTR_BONES_TABLE;
    int32_t PTR_FACES_TABLE;

    int16_t NUM_FGROUPS;
    int16_t NUM_MESH;
    int16_t NUM_BONES;
    int16_t NUM_UNK03;

    //	helper
    MeshHeader* getMeshHeader(int meshIdx);
    ComponentDataBlob* getComponentBlob(int meshIdx);
    void* getComponentData(int meshIdx);
    FaceHeader* getFaceHeader(int faceIdx);
    uint16_t* getIndexBuffer(int faceIdx);
};
struct fileHeader
{
    int32_t FILE_BYTE;
    uint32_t unknown[3];
    int32_t NR_OF_MESHES;
    uint32_t unknown2;
    uint32_t PTR_MESH_START;
    BoneHeader * getBoneHeader();
    SceneHeader * getSceneHeader();
    size_t getMeshHeaderOffset();
};
#pragma pack(pop)

class MeshPlugin :
    public PluginInterface
{
public:
    MeshPlugin();
    ~MeshPlugin();
    virtual int check(void*, size_t, CDRM_TYPES& type);
    virtual int unpack(void*, size_t, void**, size_t&, CDRM_TYPES&);
    virtual int pack(void*, size_t, void**, size_t&, CDRM_TYPES&);
    virtual int getType(void*, size_t);
private:
    XMFLOAT2 uvParser(int16_t *pTex);
    XMFLOAT3 normParser(uint8_t* pNorm);
    //	add material if it doesn't exist
    XMLElement * AddMaterial(XMLDocument &xmlScene, XMLElement * scene, string matName, string diffTexName);
    XMLElement * AddTexture(XMLDocument &xmlScene, XMLElement * scene, string diffTexName);
    XMLElement * AddEffect(XMLDocument &xmlScene, XMLElement * scene, string effectName, string techniqueName, string diffTexName);
};

API int createPluginInterface(PluginInterface** ppPluginInterface);
API int destroyPluginInterface(PluginInterface* pPluginInterface);
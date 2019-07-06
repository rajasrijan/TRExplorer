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
#include <string>
#include <vector>
#include <iterator>
#include <functional>
#include <math.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "tinyxml2.h"

#ifndef _countof
#define _countof(x) (sizeof(x) / sizeof(x[0]))
#endif

using namespace std;
using namespace tinyxml2;

#pragma pack(push, 1)

struct float2 {
    union {
        struct {
            float x, y;
        };
        float _d[2];
    };

public:
    float2(float x, float y) : _d{x, y} {}

    ~float2() {}
};

struct float3 {
    union {
        struct {
            float x, y, z;
        };
        float _d[3];
    };

public:
    float3(float x, float y, float z) : _d{x, y, z} {}

    ~float3() {}

    float3 &Normalize() {
        float    total = 0.0f;
        for (int i     = 0; i < sizeof(_d) / sizeof(_d[0]); i++) {
            total += pow(_d[i], 2);
        }
        pow(total, 0.5);
        for (int i = 0; i < sizeof(_d) / sizeof(_d[0]); i++) {
            _d[i] /= total;
        }
        return *this;
    }
};

struct float4 {
    union {
        struct {
            float x, y, z, w;
        };
        float _d[4];
    };

public:
    float4(float x, float y, float z, float w) : _d{x, y, z, w} {}

    ~float4() {}
};

struct int4 {
    union {
        struct {
            int x, y, z, w;
        };
        int _d[4];
    };

public:
    int4(int x, int y, int z, int w) : _d{x, y, z, w} {}

    ~int4() {}
};

struct FaceHeader {
    uint32_t unknown[4];
    int32_t indexFrom;
    int32_t numFaces;
    uint32_t unknown2[4];
    int32_t matNum;
    uint32_t unknown3[9];

    size_t getIndexBufferOffset();
};

struct BoneHeader {
    uint32_t unknown;
    uint32_t offsetIBoneCount;
    uint32_t offsetIBoneStart;
    uint32_t offsetIUnknCount;
    uint32_t offsetIUnknStart;

    uint32_t getBoneCountOffset();

    uint32_t getBoneStartOffset();
};

// struct Bone
// {
//     uint32_t unknown[8];
//     float x, y, z;
//     uint32_t unknown1[3];
//     int32_t pid;
//     uint32_t unknown3;
// };

struct VertexType {
    uint32_t hash;
    const char *name;
    uint32_t size;
};

enum ComponentConstant {
    CMP_Offset_Normal = 0x36f5e414,        //  5 Normals + 0x64a86f01 + 0xf1ed11c3
    CMP_Offset_UnknownMatrix = 0x3e7f6149, //  3 ?? (16)
    CMP_Offset_BoneWeight = 0x48e691c0,    //  6 BoneWeight
    CMP_Offset_BoneIndex = 0x5156d8d3,     //  7 BoneIndicies
    CMP_Offset_Unknown1 = 0x64a86f01,
    CMP_Offset_Unknown3 = 0x7e7dd623,
    CMP_Offset_TexCord = 0x8317902a,  //  19 TexCoord
    CMP_Offset_TexCord2 = 0x8e54b6f3, //  19 TexCoord
    CMP_Offset_Position = 0xd2f7d823, //  0 Position
    CMP_Offset_Unknown2 = 0xf1ed11c3,
};

VertexType VertexTypeMap[] = {{CMP_Offset_Normal, "Normal", 4}, {CMP_Offset_UnknownMatrix, "UnknownMatrix", 16}, {CMP_Offset_BoneWeight, "BoneWeight", 4}, {CMP_Offset_BoneIndex, "BoneIndex", 4}, {CMP_Offset_Unknown1, "Unknown1", 4}, {CMP_Offset_TexCord, "Texcoord", 4}, {CMP_Offset_TexCord2, "Texcoord2", 4}, {CMP_Offset_Position, "Position", 12}, {CMP_Offset_Unknown2, "Unknown2", 4}, {CMP_Offset_Unknown3, "Unknown3", 4},};

struct ComponentValue {
    uint32_t itemOffsetType;
    int16_t itemValue;
    int16_t itemType;
};

struct ComponentDataBlob {
    uint32_t unknown[2]; //Skip past unknown 0x8
    uint16_t itemCount;  //itemCount
    uint16_t vertexSize; //vertexSize
    uint32_t unknown2;

    ComponentValue *getComponents(int componentIdx);
};

template<class Type>
class DynamicStructIterator {
    uint32_t it, end, offset, size;
    void *ptr;

public:
    DynamicStructIterator(void *_ptr, uint32_t _it, uint32_t _end, uint32_t _offset, uint32_t _size) : ptr(_ptr), it(_it), end(_end), offset(_offset), size(_size) {
    }

    ~DynamicStructIterator() {}

    DynamicStructIterator &operator++() {
        it++;
    }

    bool operator!=(const DynamicStructIterator &b) const {
        return it != b.it;
    }

    Type &operator*() {
        return *((Type *) ((char *) ptr + (it * size) + offset));
    }
};

struct MeshHeader {
    int32_t m_iNumIndexGroups;
    uint16_t unknown;
    uint16_t m_uiBoneMapCount;
    int32_t BoneMapOffset;
    int32_t vertOffset;
    uint32_t unknown2[3];
    int32_t vfOffset;
    int32_t m_iNumVerts;
    int32_t unknown3;
    int32_t facesOffset;
    int32_t numFaces;
};

struct SceneHeader {
    union {
        uint32_t magic;
        char magicStr[4];
    };
    //	4 bytes
    uint32_t unknown[2];
    //	12 bytes
    uint32_t NUM_TRIS;
    //	16 bytes
    uint32_t unknown2[25]; // 100 bytes of unknown
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
    MeshHeader *getMeshHeader(int meshIdx);

    int32_t *getBoneMap(int meshIdx);

    ComponentDataBlob *getComponentBlob(int meshIdx);

    void *getComponentData(int meshIdx);

    FaceHeader *getFaceHeader(int faceIdx);

    uint16_t *getIndexBuffer(int faceIdx);

    bool HasBones();
};

#pragma pack(pop)

class Mesh {
public:
    std::vector<glm::vec3>             vertBuff;
    std::vector<glm::vec4>             vertBuff2;
    std::vector<glm::vec3>             normBuff;
    std::vector<glm::vec2>             uvBuff;
    std::vector<uint8_t>               uv2Buff;
    std::vector<glm::vec4>             bwBuff;
    std::vector<glm::vec4>             biBuff;
    //  face groupes
    std::vector<std::vector<uint16_t>> idxBuff;
    std::vector<size_t>                matList;
    int                                numVerts;
    int                                numFaces;
    int                                numIdx;
    string                             matName;
    string                             meshName;
    int                                unkn_pos;
    int                                xyz_pos;
    int                                xyz_pos2;
    int                                norm_pos;
    int                                norm_pos2;
    int                                norm_pos3;
    int                                uv_pos;
    int                                uv2_pos;
    int                                bw_pos;
    int                                bi_pos;
    int                                BoneMapOffset;
    int                                meshOffset;
    int                                vertOffset;
    int                                vfOffset;
    int                                facesOffset;
    int                                numFaceGroups;
    vector<size_t>                     boneMap;
    std::string                        name, id;

    Mesh();
};

enum BitStreamSeek {
    SEEK_ABS, SEEK_REL
};

class Bone {
public:
    int       bone_id;
    string    bone_name;
    glm::mat4 world;
    int       parent_id;

    Bone() {}

    Bone(int bid, string bname, const glm::mat4 &mat, void *ptr, int pid) : bone_id(bid), bone_name(bname), world(mat), parent_id(pid) {}
};

class BitStreamWriter {
    std::vector<uint8_t> data;
    size_t               offset;

    void updateSize(size_t new_size) {
        if (new_size > data.size()) {
            data.resize(new_size);
        }
    }

public:
    BitStreamWriter() : offset(0) {

    }

    template<class T>
    void write(T t) {
        updateSize(offset + sizeof(T));
        auto dataPtr = data.data();
        ((T *) &dataPtr[offset])[0] = t;
        offset += sizeof(T);
    }

    void seek(int off, BitStreamSeek type) {
        if (type == SEEK_ABS) {
            offset = off;
            updateSize(offset);
        } else if (type == SEEK_REL) {
            offset += off;
            updateSize(offset);
        } else {
            throw runtime_error("Unknown BitStreem seek type!");
        }
    }

    size_t getOffset() {
        return offset;
    }

    size_t size() {
        return data.size();
    }

    const void *getData() {
        return data.data();
    }
};

class BitStream {
    char   *dataPtr;
    size_t size;
    size_t offset;

public:
    BitStream(char *ptr, size_t sz) : dataPtr(ptr), size(sz), offset(0) {
    }

    template<typename t>
    t read() {
        t tmp = ((t *) (dataPtr + offset))[0];
        offset += sizeof(t);
        return tmp;
    }

    std::vector<char> readBytes(size_t sz) {
        std::vector<char> result(dataPtr + offset, dataPtr + offset + sz);
        offset += sz;
        return result;
    }

    void seek(size_t off, BitStreamSeek type) {
        if (type == SEEK_ABS) {
            offset = off;
        } else if (type == SEEK_REL) {
            offset += off;
        } else {
            throw runtime_error("Unknown BitStreem seek type!");
        }
    }

    size_t getOffset() { return offset; }
};

class MeshPlugin : public PluginInterface {

public:
    MeshPlugin();

    ~MeshPlugin();

    virtual int check(void *, size_t, CDRM_TYPES &type);

    virtual int unpack(void *, size_t, void **, size_t &, CDRM_TYPES &);

    virtual int pack(void *, size_t, void **, size_t &, CDRM_TYPES &);

    virtual int getType(void *, size_t) { return CDRM_TYPE_MESH; }

private:
    std::string serialize(const std::vector<Mesh> &meshList, const std::vector<Bone> &boneList);

    void asset(tinyxml2::XMLDocument &collada, XMLElement *rootElement);

    // void library_images(tinyxml2::XMLDocument &collada,const std::vector<Mesh> &meshList);
    // void library_effects(tinyxml2::XMLDocument &collada,const std::vector<Mesh> &meshList);
    void library_materials(tinyxml2::XMLDocument &collada, XMLElement *rootElement, const std::vector<Mesh> &meshList);

    void controller(tinyxml2::XMLDocument &collada, XMLElement *rootElement, const Mesh &mesh, const std::vector<Bone> &boneList);

    void geometry(tinyxml2::XMLDocument &collada, XMLElement *rootElement, const Mesh &mesh);

    void library_controllers(tinyxml2::XMLDocument &collada, XMLElement *rootElement, const std::vector<Mesh> &meshList, const std::vector<Bone> &boneList);

    void library_geometries(tinyxml2::XMLDocument &collada, XMLElement *rootElement, const std::vector<Mesh> &meshList);

    void library_visual_scenes(tinyxml2::XMLDocument &collada, XMLElement *rootElement, const std::vector<Bone> &boneList);
};

API int createPluginInterface(PluginInterface **ppPluginInterface);
API int destroyPluginInterface(PluginInterface *pPluginInterface);
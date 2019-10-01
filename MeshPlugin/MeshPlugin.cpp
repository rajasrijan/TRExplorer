/*
MIT License

Copyright (c) 2018 Srijan Kumar Sharma

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

#include <stddef.h>
#include <stdint.h>
#include "MeshPlugin.h"
#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <algorithm>
#include <functional>
#include <array>
#include <chrono>
#include <memory>
#include <float.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <map>
#include <set>
#include <queue>
#include <algorithm>
#include <iomanip>
#include "tinyxml2.h"

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
using namespace tinyxml2;

#define SCENE_MAGIC (0x6873654D)

int createPluginInterface(PluginInterface **ppPluginInterface, const PluginCreateInfo &pluginCreateInfo) {
    if (!ppPluginInterface) {
        return 1;
    }
    *ppPluginInterface = new MeshPlugin(pluginCreateInfo.gameVersion);
    if (!*ppPluginInterface) {
        return 1;
    }
    return 0;
}

int destroyPluginInterface(PluginInterface *pPluginInterface) {
    if (!pPluginInterface) {
        return 1;
    }
    delete pPluginInterface;
    return 0;
}

Mesh::Mesh() : numVerts(0), numFaces(0), numIdx(0), matName(""), meshName(""), unkn_pos(-1), xyz_pos(-1), xyz_pos2(-1), norm_pos(-1), norm_pos2(-1), norm_pos3(-1), uv_pos(-1), uv2_pos(-1), bw_pos(-1), bi_pos(-1), BoneMapOffset(0), meshOffset(0), vertOffset(0), vfOffset(0), facesOffset(0), numFaceGroups(0) {
}

MeshPlugin::MeshPlugin(uint32_t gameVer) : PluginInterface(gameVer) {
}

MeshPlugin::~MeshPlugin() {
}

static inline void dumpData(string fn, char *pData, size_t sz) {
    fstream f(fn, ios_base::binary | ios_base::out);
    f.write(pData, sz);
}

//check if it's this type based on the data
int MeshPlugin::check(void *pData, size_t sz, CDRM_TYPES &type) {
    if (sz < 28) {
        return 0;
    }
    if (((uint32_t *) pData)[0] != 0x6 && ((uint32_t *) pData)[0] != 0x59) {
        return 0;
    }
    // if (pMeshHeader->magic != SCENE_MAGIC)
    // {
    //     return 0;
    // }
    type = CDRM_TYPE_MESH;
    return 1;
}

std::string get_datetime_string(std::chrono::system_clock::time_point t) {
    char      some_buffer[256] = {0};
    auto      as_time_t        = std::chrono::system_clock::to_time_t(t);
    struct tm tm;
    if (::gmtime_r(&as_time_t, &tm)) {
        if (std::strftime(some_buffer, sizeof(some_buffer), "%FT%TZ", &tm)) {
            return std::string{some_buffer};
        }
    }
    throw std::runtime_error("Failed to get current date as string");
}

glm::vec2 uvParser(short *bytes) {

    glm::vec2   uvList(0, 0);
    for (size_t i = 0; i < 2; i++) {
        uvList[i] = (float) bytes[i] / 2048.0f;
    }
    return uvList;
}

glm::vec3 normParser(uint8_t *bytes) {
    glm::vec3 normList(0, 0, 0);
    for (int  n = 0; n < 3; n++) {
        normList[n] = (255 * bytes[n]) - (128 - bytes[3]);
    }
    normList = glm::vec3(normList[0], normList[2], normList[1]); // YZ Swap
    return glm::normalize(normList);
}

glm::vec4 boneParser(uint8_t *bytes) {
    glm::vec4 boneList;
    for (int  i = 0; i < 4; i++) {
        boneList[i] = (float) bytes[i] / 255.0;
    }
    return boneList;
}

glm::vec4 boneMapParser(const std::vector<size_t> &boneMap, uint8_t *bytes) {

    glm::vec4 boneMapList;
    for (int  i = 0; i < 4; i++) {
        boneMapList[i] = boneMap[bytes[i]];
    }
    return boneMapList;
}

void multiplyBones(std::vector<Bone> &boneList, size_t parent = 0) {
    std::sort(boneList.begin(), boneList.end(), [](const Bone &lhs, const Bone &rhs) { return rhs.bone_id > lhs.bone_id; });
    for (size_t i = 0; i < boneList.size(); i++) {
        auto &bone = boneList[i];
        if (bone.parent_id == -1) {
            continue;
        }
        bone.world = boneList[bone.parent_id].world * bone.world;
    }
}

int MeshPlugin::unpack(void *pDataIn, size_t szIn, void **ppDataOut, size_t &szOut, CDRM_TYPES &type) {
    //ctx = rapi.rpgCreateContext()
    auto bs = BitStream((char *) pDataIn, szIn);

    std::vector<Mesh> meshList;
    std::vector<Bone> boneList;

    int faceGroupIndex = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    auto FILE_BYTE = bs.read<int>();
    bs.seek(16, SEEK_ABS);
    auto NR_OF_MESHES = bs.read<int>();
    bs.seek(24, SEEK_ABS);
    auto PTR_MESH_START = bs.read<int>();
    bs.seek((FILE_BYTE * 8), SEEK_ABS);
    auto HEADER_END = bs.getOffset();
    bs.seek(20, SEEK_REL); // Bone Pointers
    bs.seek((NR_OF_MESHES * 4), SEEK_REL);

    auto BASE_START = bs.getOffset();
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    bs.seek(BASE_START + PTR_MESH_START, SEEK_ABS);

    auto MESH_START = bs.getOffset();

    bs.seek(12, SEEK_REL);
    auto NUM_TRIS = bs.read<unsigned int>();
    bs.seek(100, SEEK_REL);
    auto PTR_MAT_TABLE    = bs.read<int>();
    auto PTR_MESH_TBL00   = bs.read<int>();
    auto PTR_BONES_TABLE1 = bs.read<int>();
    auto PTR_BONES_TABLE  = bs.read<int>();
    auto PTR_FACES_TABLE  = bs.read<int>();

    auto      NUM_FGROUPS = bs.read<short>();
    auto      NUM_MESH    = bs.read<short>();
    auto      NUM_BONES   = bs.read<short>();
    auto      NUM_UNK03   = bs.read<short>();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Parse Bones
    if (NUM_BONES > 0) {
        bs.seek(HEADER_END, SEEK_ABS);

        auto UNKN           = bs.read<int>();
        auto PTR_BONE_COUNT = bs.read<int>() - 4;
        auto PTR_BONE_START = bs.read<int>();
        auto PTR_UNKN_COUNT = bs.read<int>() - 4;
        auto PTR_UNKN_START = bs.read<int>();

        bs.seek(BASE_START + PTR_BONE_COUNT, SEEK_ABS);
        auto BONE_COUNT = bs.read<int>();

        if (BONE_COUNT == NUM_BONES) {
            bs.seek(BASE_START + PTR_BONE_START, SEEK_ABS);
            for (auto i = 0; i < BONE_COUNT; i++) {
                bs.seek(32, SEEK_REL);
                auto BONE_XPOS = bs.read<float>();
                auto BONE_YPOS = bs.read<float>();
                auto BONE_ZPOS = bs.read<float>();
                bs.seek(12, SEEK_REL);
                auto BONE_PID = bs.read<int>();
                bs.seek(4, SEEK_REL);
                glm::mat4 mat = glm::mat4(1);
                mat[3] = glm::vec4(-BONE_XPOS, -BONE_YPOS, -BONE_ZPOS, 1);
                boneList.push_back(Bone(i, "b_" + to_string(i), mat, nullptr, BONE_PID));
            }
            // global coordinate
            //  multiplyBones(boneList);
        }
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        bs.seek(BASE_START + PTR_UNKN_COUNT, SEEK_ABS);
        auto UNKN_COUNT = bs.read<int>();

        if (UNKN_COUNT > 0) {
            bs.seek(BASE_START + PTR_UNKN_START, SEEK_ABS);
            auto unknownStuff = bs.readBytes(UNKN_COUNT * 4);
        }
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Parse Mesh Desc
    for (auto m           = 0; m < NUM_MESH; m++) {

        bs.seek((MESH_START + PTR_MESH_TBL00) + (m * 48), SEEK_ABS);

        auto mesh = Mesh();
        mesh.name          = string("M_") + to_string(m);
        mesh.id            = string("Mesh_") + to_string(m);
        mesh.numFaceGroups = bs.read<int>();
        bs.seek(2, SEEK_REL);
        auto BoneMapCount = bs.read<short>();
        mesh.BoneMapOffset = bs.read<int>();
        mesh.vertOffset    = bs.read<int>();
        bs.seek(12, SEEK_REL);
        mesh.vfOffset = bs.read<int>();
        mesh.numVerts = bs.read<int>();
        bs.seek(4, SEEK_REL);
        mesh.facesOffset = bs.read<int>();
        mesh.numFaces    = bs.read<int>();
        mesh.numIdx      = mesh.numFaces * 3;

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Parse Bone Map
        if (NUM_BONES > 0) {
            bs.seek(MESH_START + mesh.BoneMapOffset, SEEK_ABS);
            for (auto i = 0; i < BoneMapCount; i++) {
                mesh.boneMap.push_back(bs.read<int>());
            }
        }
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Parse Vertex Format
        bs.seek(MESH_START + mesh.vfOffset, SEEK_ABS); //Skip to FVF Table
        bs.seek(8, SEEK_REL);                          //Skip past unknown 0x8
        auto itemCount  = bs.read<short>();             //itemCount
        auto vertexSize = bs.read<short>();            //vertexSize
        bs.seek(4, SEEK_REL);

        for (auto i = 0; i < itemCount; i++) {
            auto itemOffset = bs.read<unsigned int>();
            auto itemValue  = bs.read<short>();
            auto itemType   = bs.read<short>();
            if (itemOffset == 0xd2f7d823) { //0 Position
                mesh.xyz_pos = itemValue;
            } else if (itemOffset == 0x8317902A) { //19 TexCoord
                mesh.uv_pos = itemValue;
            } else if (itemOffset == 0x8E54B6F3) { //19 TexCoord2
                mesh.uv2_pos = itemValue;
            } else if (itemOffset == 0x48e691c0) { //6 BoneWeight
                mesh.bw_pos = itemValue;
            } else if (itemOffset == 0x5156d8d3) { //7 BoneIndicies
                mesh.bi_pos = itemValue;
            } else if (itemOffset == 0x36f5e414) { //5 Normals
                mesh.norm_pos = itemValue;
            } else if (itemOffset == 0x3e7f6149) { //3 tessVert
                // 16 bytes
                mesh.xyz_pos2 = itemValue;
                printf("Tess ignored till I know what to do!\n");
            } else if (itemOffset == 0X64A86F01) { //5 BiNormal
                // 4 bytes
                mesh.norm_pos2 = itemValue;
            } else if (itemOffset == 0xf1ed11c3) { //5 Tangent
                // 4 bytes
                mesh.norm_pos3 = itemValue;
            } else if (itemOffset == 0X7E7DD623) { //4 color1?
                // 4 bytes
                printf("%s. Unknown item\n", mesh.name.c_str());
                mesh.unkn_pos = itemValue;
            } else {
                printf(KRED "ERROR: Vertex attribute not defined! " KNRM);
            }
            printf("Vertex attribute [%#X], type [%#X], offset [%#X]\n", itemOffset, itemType, itemValue);
        }
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //Build Mesh
        bs.seek(MESH_START + mesh.vertOffset, SEEK_ABS);
        auto buffer = bs.readBytes(mesh.numVerts * vertexSize);

        for (auto v = 0; v < mesh.numVerts; v++) {
            auto idx  = v * vertexSize;
            auto *tmp = (float *) &buffer[idx + mesh.xyz_pos];
            mesh.vertBuff.push_back(glm::vec3(tmp[0], tmp[1], tmp[2]));
            if (mesh.xyz_pos2 != -1) {
                auto *tmp = (float *) &buffer[idx + mesh.xyz_pos2];
                mesh.vertBuff2.push_back(glm::vec4(tmp[0], tmp[1], tmp[2], tmp[3]));
            }
            if (mesh.uv_pos != -1) {
                auto *tmp = (short *) &buffer[idx + mesh.uv_pos];
                mesh.uvBuff.push_back(uvParser(tmp));
            }
            if (mesh.norm_pos != -1) {
                auto *tmp = (uint8_t *) &buffer[idx + mesh.norm_pos];
                mesh.normBuff.push_back(normParser(tmp));
            }
            if (mesh.bw_pos != -1) {
                auto *tmp = (uint8_t *) &buffer[idx + mesh.bw_pos];
                mesh.bwBuff.push_back(boneParser(tmp));
            }
            if (mesh.bi_pos != -1) {
                auto *tmp = (uint8_t *) &buffer[idx + mesh.bi_pos];
                mesh.biBuff.push_back(boneMapParser(mesh.boneMap, tmp));
            }
        }
        buffer.clear();
        if (NR_OF_MESHES == 1 and mesh.numFaces * 2 == mesh.numVerts) { // Spline
            bs.seek(MESH_START + PTR_FACES_TABLE, SEEK_ABS);
            bs.seek(mesh.facesOffset * sizeof(unsigned short), SEEK_REL);
            auto tmp = bs.readBytes(mesh.numVerts * sizeof(unsigned short));
            mesh.idxBuff.insert(mesh.idxBuff.end(), vector<uint16_t>((uint16_t *) tmp.data(), (uint16_t *) tmp.data() + mesh.numVerts));
        } else {
            // split mesh into facegroups
            for (auto g = 0; g < mesh.numFaceGroups; g++) {
                bs.seek((MESH_START + PTR_MAT_TABLE) + (faceGroupIndex * 80), SEEK_ABS);
                bs.seek(16, SEEK_REL);
                auto indexFrom = bs.read<int>();
                auto numFaces  = bs.read<int>();
                bs.seek(16, SEEK_REL);
                auto matNum = bs.read<int>();
                bs.seek(36, SEEK_REL);

                faceGroupIndex += 1;
                auto numIdx = numFaces * 3;

                bs.seek(MESH_START + PTR_FACES_TABLE, SEEK_ABS);
                bs.seek(indexFrom * sizeof(unsigned short), SEEK_REL);
                auto idxBuff = bs.readBytes(numIdx * 2);
                mesh.idxBuff.insert(mesh.idxBuff.end(), vector<uint16_t>((uint16_t *) idxBuff.data(), ((uint16_t *) idxBuff.data()) + numIdx));
                mesh.matList.push_back(matNum);
            }
        }
        meshList.push_back(mesh);
        //rapi.rpgClearBufferBinds();
    }
    // auto mdl = rapi.rpgConstructModel();
    // mdl.setModelMaterials(NoeModelMaterials(texList, matList));
    // mdlList.push_back(mdl);
    // mdl.setBones(boneList);
    auto      xml         = serialize(meshList, boneList);
    ppDataOut[0] = strdup(xml.c_str());
    szOut = xml.size() + 1;
    return 0;
}

int MeshPlugin::pack(void *buffer, size_t sz, void **data, size_t &sz_data, CDRM_TYPES &type) {
    const aiScene    *scene = nullptr;
    Assimp::Importer importer;
    scene                                   = importer.ReadFileFromMemory(buffer, sz, 0);
    if (!scene) {
        printf("Invalid scene\n");
    }
    map<string, vector<aiMesh *>> meshGroups;
    for (int                      meshId    = 0; meshId < scene->mNumMeshes; ++meshId) {
        string meshName = scene->mMeshes[meshId]->mName.C_Str();
        meshGroups[meshName].push_back(scene->mMeshes[meshId]);
    }
    vector<Bone>                  bones;
    {
        int             maxBoneId     = 0;
        map<int, Bone>  boneMap;
        queue<aiNode *> stack;
        auto            skeleton_root = scene->mRootNode->FindNode("skeleton_root");
        for_each(skeleton_root->mChildren, skeleton_root->mChildren + skeleton_root->mNumChildren, [&stack](auto child) { stack.push(child); });
        while (stack.size() > 0) {
            auto node = stack.front();
            stack.pop();
            int boneId      = 0, parentBoneId = -1;
            sscanf(node->mName.C_Str(), "Armature_bone%d", &boneId);
            sscanf(node->mParent->mName.C_Str(), "Armature_bone%d", &parentBoneId);
            glm::mat4 world;
            for (int  i     = 0; i < 4; i++) {
                for (int j = 0; j < 4; ++j) {
                    world[i][j] = node->mTransformation[i][j];
                }
            }
            boneMap[boneId] = Bone(boneId, node->mName.C_Str(), world, nullptr, parentBoneId);
            for_each(node->mChildren, node->mChildren + node->mNumChildren, [&stack](auto child) { stack.push(child); });
            maxBoneId = max(maxBoneId, boneId);
        }
        bones.resize(maxBoneId + 1);
        for (int i = 0; i < maxBoneId + 1; ++i) {
            bones[i] = boneMap[i];
        }
    }
    auto                          bs        = BitStreamWriter();
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    int                           FILE_BYTE = 0x59;
    bs.write(FILE_BYTE);
    bs.seek(16, SEEK_ABS);
    int unknownCount = 0x17;
    bs.write(unknownCount);
    bs.seek(24, SEEK_ABS);
    int PTR_MESH_START = (FILE_BYTE * 8) + (64 * bones.size()) + 20 + (unknownCount * 4) + 4;
    bs.write(PTR_MESH_START);
    bs.seek((FILE_BYTE * 8), SEEK_ABS);
    auto HEADER_END = bs.getOffset();
    bs.seek(20, SEEK_REL); // Bone Pointers
    bs.seek((unknownCount * 4), SEEK_REL);
    auto BASE_START = bs.getOffset();
    //  bone data
    int  UNKN       = 1970;
    int  NUM_BONES  = bones.size();
    bs.write(NUM_BONES);
    int             PTR_BONE_COUNT = bs.getOffset() - BASE_START;
    int             PTR_BONE_START = bs.getOffset() - BASE_START;
    for (const auto &bone:bones) {
        bs.seek(32, SEEK_REL);
        bs.write<float>(-bone.world[0][3]);
        bs.write<float>(-bone.world[1][3]);
        bs.write<float>(-bone.world[2][3]);
        bs.seek(12, SEEK_REL);
        bs.write<int>(bone.parent_id);
        bs.seek(4, SEEK_REL);
    }
    int             PTR_UNKN_COUNT = bs.getOffset() + 4;
    int             PTR_UNKN_START = bs.getOffset();
    uint32_t        NUM_TRIS       = 0;

    bs.seek(HEADER_END, SEEK_ABS);
    bs.write(UNKN);
    bs.write(PTR_BONE_COUNT);
    bs.write(PTR_BONE_START);
    bs.write(PTR_UNKN_COUNT);
    bs.write(PTR_UNKN_START);

    bs.seek(BASE_START + PTR_MESH_START, SEEK_ABS);
    int MESH_START = bs.getOffset();
    bs.seek(12, SEEK_REL);
    bs.write(NUM_TRIS);

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    sz_data = bs.size();
    data[0] = new char[sz_data];
    memcpy(data[0], bs.getData(), sz_data);
    return 0;
}

XMLElement *createElement(XMLDocument &collada, XMLElement *rootElement, const string &name, const string &value = string(), const vector<pair<string, string>> &attributes = vector<pair<string, string>>(), XMLElement *source = nullptr) {
    XMLElement *tmp = (XMLElement *) rootElement->InsertEndChild(collada.NewElement(name));
    if (!value.empty()) {
        tmp->SetText(value);
    }
    for (const auto &val : attributes) {
        tmp->SetAttribute(val.first.c_str(), val.second);
    }
    if (source) {
        tmp->SetAttribute("source", string("#") + source->Attribute("id"));
    }
    return tmp;
}

void MeshPlugin::asset(XMLDocument &collada, XMLElement *rootElement) {
    XMLElement *asset       = createElement(collada, rootElement, "asset");
    XMLElement *contributor = createElement(collada, asset, "contributor");
    XMLElement *author      = createElement(collada, contributor, "author", "MeshPlugin");
    auto       unit         = createElement(collada, asset, "unit", string(), {{"meter", "0.01"}, {"name", "centimeter"}});
    auto       up_axis      = createElement(collada, asset, "up_axis", "Z_UP");
}

template<int c>
void source(tinyxml2::XMLDocument &collada, XMLElement *rootElement, const std::vector<glm::vec<c, float, glm::highp>> &data) {
    stringstream ss;
    ss << "\n";
    for (int j = 0; j < data.size(); j++) {
        for (int i = 0; i < c; i++) {
            ss << setprecision(19) << data[j][i] << " ";
        }
    }
    ss << "\n";
    auto float_array      = createElement(collada, rootElement, "float_array", ss.str(), {{"count", to_string(data.size() * c)}, {"id", rootElement->Attribute("id") + string("-array")}});
    auto technique_common = createElement(collada, rootElement, "technique_common");
    auto accessor         = createElement(collada, technique_common, "accessor", string(), {{"count", to_string(data.size())}, {"stride", to_string(c)}}, float_array);
    if (c == 4) {
        createElement(collada, accessor, "param", string(), {{"type", "float"}, {"name", "X"}});
        createElement(collada, accessor, "param", string(), {{"type", "float"}, {"name", "Y"}});
        createElement(collada, accessor, "param", string(), {{"type", "float"}, {"name", "Z"}});
        createElement(collada, accessor, "param", string(), {{"type", "float"}, {"name", "W"}});
    } else if (c == 3) {
        createElement(collada, accessor, "param", string(), {{"type", "float"}, {"name", "X"}});
        createElement(collada, accessor, "param", string(), {{"type", "float"}, {"name", "Y"}});
        createElement(collada, accessor, "param", string(), {{"type", "float"}, {"name", "Z"}});
    } else if (c == 2) {
        createElement(collada, accessor, "param", string(), {{"type", "float"}, {"name", "S"}});
        createElement(collada, accessor, "param", string(), {{"type", "float"}, {"name", "T"}});
    } else if (c == 1) {
        createElement(collada, accessor, "param", string(), {{"type", "float"}, {"name", "WEIGHT"}});
    } else {
        throw runtime_error("Unknown Type");
    }
}

std::ostream &operator<<(std::ostream &out, glm::mat4x4 const &mat) {
    for (size_t i = 0; i < 4; i++) {
        for (size_t j = 0; j < 4; j++) {
            out << std::setprecision(19) << mat[j][i] << " ";
        }
    }
    return out;
}

std::ostream &operator<<(std::ostream &out, glm::vec4 const &vec) {
    for (size_t i = 0; i < 4; i++) {
        out << std::setprecision(19) << vec[i] << " ";
    }
    return out;
}

template<class T>
int calculateStride() {
    if (typeid(T) == typeid(glm::mat4x4)) {
        return 16;
    } else {
        throw runtime_error("Unknown Type");
    }
    return -1;
}

template<class T>
void source(tinyxml2::XMLDocument &collada, XMLElement *rootElement, const std::vector<T> &data) {
    int          stride = calculateStride<T>();
    stringstream ss;
    ss << "\n";
    for (const auto &d : data) {
        ss << setprecision(19) << d << "\n";
    }
    ss << "\n";
    auto float_array      = createElement(collada, rootElement, "float_array", ss.str(), {{"count", to_string(data.size() * stride)}, {"id", rootElement->Attribute("id") + string("-array")}});
    auto technique_common = createElement(collada, rootElement, "technique_common");
    auto accessor         = createElement(collada, technique_common, "accessor", string(), {{"count", to_string(data.size())}, {"stride", to_string(stride)}}, float_array);
    if (typeid(T) == typeid(glm::mat4x4)) {
        createElement(collada, accessor, "param", string(), {{"type", "float4x4"}});
    } else {
        throw runtime_error("Unknown Type");
    }
}

void source(tinyxml2::XMLDocument &collada, XMLElement *rootElement, const std::vector<Bone> &data) {
    stringstream ss;
    ss << "\n";
    for (int i = 0; i < data.size(); i++) {
        ss << data[i].bone_name << " ";
    }
    ss << "\n";
    auto Name_array       = createElement(collada, rootElement, "Name_array", ss.str(), {{"count", to_string(data.size())}, {"id", rootElement->Attribute("id") + string("-array")}});
    auto technique_common = createElement(collada, rootElement, "technique_common");
    auto accessor         = createElement(collada, technique_common, "accessor", string(), {{"count", to_string(data.size())}, {"stride", "1"}}, Name_array);
    createElement(collada, accessor, "param", string(), {{"type", "name"}, {"name", "JOINT"}});
}

void MeshPlugin::geometry(tinyxml2::XMLDocument &collada, XMLElement *rootElement, const Mesh &mesh) {
    auto geometry       = createElement(collada, rootElement, "geometry", string(), {{"id", mesh.id}, {"name", mesh.name}});
    auto meshNode       = createElement(collada, geometry, "mesh");
    //
    auto sourcePosition = createElement(collada, meshNode, "source", string(), {{"id", mesh.id + "-Pos"}});
    source(collada, sourcePosition, mesh.vertBuff);
    //
    auto sourcePosition2 = createElement(collada, meshNode, "source", string(), {{"id", mesh.id + "-Pos2"}});
    source(collada, sourcePosition2, mesh.vertBuff2);
    //
    auto sourceNormal = createElement(collada, meshNode, "source", string(), {{"id", mesh.id + "-Norm"}});
    source(collada, sourceNormal, mesh.normBuff);
    //
    auto sourceTex = createElement(collada, meshNode, "source", string(), {{"id", mesh.id + "-Tex"}});
    source(collada, sourceTex, mesh.uvBuff);
    //
    auto vertices = createElement(collada, meshNode, "vertices", string(), {{"id", mesh.id + "-Vertex"}});
    createElement(collada, vertices, "input", string(), {{"semantic", "POSITION"}}, sourcePosition);
    for (int faceId = 0; faceId < mesh.idxBuff.size(); faceId++) {
        const auto &idxBuff = mesh.idxBuff[faceId];
        auto       polylist = createElement(collada, meshNode, "polylist", string(), {{"material", "material" + to_string(mesh.matList[faceId])}, {"count", to_string(idxBuff.size() / 3)}});
        createElement(collada, polylist, "input", string(), {{"offset", "0"}, {"semantic", "VERTEX"}}, vertices);
        createElement(collada, polylist, "input", string(), {{"offset", "1"}, {"semantic", "NORMAL"}}, sourceNormal);
        createElement(collada, polylist, "input", string(), {{"offset", "2"}, {"semantic", "TEXCOORD"}}, sourceTex);
        stringstream ss;
        for (int     i      = 0; i < idxBuff.size() / 3; i++) {
            ss << "3 ";
        }
        auto         vcount = createElement(collada, polylist, "vcount", ss.str());
        ss = stringstream();
        for (int i = 0; i < idxBuff.size(); i++) {
            ss << idxBuff[i] << " " << idxBuff[i] << " " << idxBuff[i] << " ";
        }
        auto     p = createElement(collada, polylist, "p", ss.str());
    }
}

void MeshPlugin::controller(tinyxml2::XMLDocument &collada, XMLElement *rootElement, const Mesh &mesh, const std::vector<Bone> &boneList) {
    const string identity_matrix_string("1.00000 0.00000 0.00000 0.00000 0.00000 1.00000 0.00000 0.00000 0.00000 0.00000 1.00000 0.00000 0.00000 0.00000 0.00000 1.00000");
    auto         controller        = createElement(collada, rootElement, "controller", string(), {{"id", mesh.id + "-skin"}});
    auto         skin              = createElement(collada, controller, "skin", string(), {{"source", "#" + mesh.id}});
    auto         bind_shape_matrix = createElement(collada, skin, "bind_shape_matrix", identity_matrix_string);
    //  source joints
    auto         sourceJoints      = createElement(collada, skin, "source", string(), {{"id", mesh.id + "-skin-joints"}});
    source(collada, sourceJoints, boneList);
    //  source weights
    auto                         sourceWights = createElement(collada, skin, "source", string(), {{"id", mesh.id + "-skin-weights"}});
    std::vector<glm::highp_vec1> data;
    stringstream                 vcount, v;
    for (int                     boneIdx      = 0; boneIdx < mesh.biBuff.size(); ++boneIdx) {
        size_t   bone_count = 0;
        for (int bc         = 0; bc < 4; bc++) {
            if (mesh.bwBuff[boneIdx][bc] != 0) {
                v << setprecision(19) << mesh.biBuff[boneIdx][bc] << " " << data.size() << " ";
                data.push_back(glm::highp_vec1(mesh.bwBuff[boneIdx][bc]));
                bone_count++;
            }
        }
        vcount << bone_count << " ";
    }
    source(collada, sourceWights, data);
    //  source poses
    auto                   sourcePoses = createElement(collada, skin, "source", string(), {{"id", mesh.id + "-skin-poses"}});
    std::vector<glm::mat4> poses;
    poses.resize(boneList.size());
    auto            poseIt = poses.begin();
    for (const auto &bone : boneList) {
        if (bone.parent_id == -1) {
            *poseIt++ = bone.world;
        } else {
            *poseIt++ = bone.world * poses[bone.parent_id];
        }
    }
    source(collada, sourcePoses, poses);
    auto joints = createElement(collada, skin, "joints");
    createElement(collada, joints, "input", string(), {{"semantic", "JOINT"}}, sourceJoints);
    createElement(collada, joints, "input", string(), {{"semantic", "INV_BIND_MATRIX"}}, sourcePoses);

    auto vertex_weights = createElement(collada, skin, "vertex_weights", string(), {{"count", to_string(mesh.biBuff.size())}});
    createElement(collada, vertex_weights, "input", string(), {{"semantic", "JOINT"}, {"offset", "0"}}, sourceJoints);
    createElement(collada, vertex_weights, "input", string(), {{"semantic", "WEIGHT"}, {"offset", "1"}}, sourceWights);

    createElement(collada, vertex_weights, "vcount", vcount.str());
    createElement(collada, vertex_weights, "v", v.str());
}

void MeshPlugin::library_controllers(tinyxml2::XMLDocument &collada, XMLElement *rootElement, const std::vector<Mesh> &meshList, const std::vector<Bone> &boneList) {
    auto      library_controllers = createElement(collada, rootElement, "library_controllers");
    for (auto &mesh : meshList) {
        controller(collada, library_controllers, mesh, boneList);
    }
}

void MeshPlugin::library_geometries(tinyxml2::XMLDocument &collada, XMLElement *rootElement, const std::vector<Mesh> &meshList) {
    auto      library_geometries = createElement(collada, rootElement, "library_geometries");
    for (auto &mesh : meshList) {
        geometry(collada, library_geometries, mesh);
    }
}

void MeshPlugin::library_visual_scenes(tinyxml2::XMLDocument &collada, XMLElement *rootElement, const std::vector<Bone> &boneList) {
    const string identity_matrix_string("1.00000 0.00000 0.00000 0.00000 0.00000 1.00000 0.00000 0.00000 0.00000 0.00000 1.00000 0.00000 0.00000 0.00000 0.00000 1.00000");
    auto         library_visual_scenes = createElement(collada, rootElement, "library_visual_scenes");
    auto         visual_scene          = createElement(collada, library_visual_scenes, "visual_scene", string(), {{"id", "Scene"}, {"name", "main_scene"}});
    auto         library_controllers   = rootElement->FirstChildElement("library_controllers");
    auto         root_skeleton         = createElement(collada, visual_scene, "node", string(), {{"name", "skeleton_root"}, {"id", "skeleton_root"}, {"type", "NODE"}});
    createElement(collada, root_skeleton, "matrix", identity_matrix_string, {{"sid", "transform"}});
    std::vector<XMLElement *> skeleton(boneList.size());
    for (size_t               i        = 0; i < boneList.size(); i++) {
        const auto &bone        = boneList[i];
        XMLElement *parent_node = nullptr;
        if (bone.parent_id == -1) {
            parent_node = root_skeleton;
        } else {
            parent_node = skeleton[bone.parent_id];
        }
        skeleton[i] = createElement(collada, parent_node, "node", string(), {{"type", "JOINT"}, {"sid", "b_" + to_string(bone.bone_id)}, {"id", "Armature_bone" + to_string(bone.bone_id)}});
        stringstream ss;
        ss << bone.world;
        createElement(collada, skeleton[i], "matrix", ss.str(), {{"sid", "transform"}});
    }
    for (auto                 it       = library_controllers->FirstChildElement("controller"); it != 0; it = it->NextSiblingElement("controller")) {
        auto node                = createElement(collada, visual_scene, "node", string(), {{"id", it->Attribute("id") + string("-scene")}, {"name", it->Attribute("id") + string("-scene")}});
        auto matrix              = createElement(collada, node, "matrix", identity_matrix_string, {{"sid", "transform"}});
        auto instance_controller = createElement(collada, node, "instance_controller", string(), {{"url", string("#") + it->Attribute("id")}});
        createElement(collada, instance_controller, "skeleton", "#Armature_bone0");
    }
    auto                      scene                                                                        = createElement(collada, rootElement, "scene");
    auto                      instance_visual_scene                                                        = createElement(collada, scene, "instance_visual_scene", string(), {{"url", "#Scene"}});
}

void MeshPlugin::library_materials(tinyxml2::XMLDocument &collada, XMLElement *rootElement, const std::vector<Mesh> &meshList) {
    auto            library_materials = createElement(collada, rootElement, "library_materials");
    set<size_t>     materials;
    for (const auto &mesh:meshList) {
        for (int i = 0; i < mesh.matList.size(); ++i) {
            materials.insert(mesh.matList[i]);
        }
    }
    for (size_t     matId:materials) {
        createElement(collada, library_materials, "material", string(), {{"id", "material" + to_string(matId)}, {"name", "material" + to_string(matId)}});
    }
}

std::string MeshPlugin::serialize(const std::vector<Mesh> &meshList, const std::vector<Bone> &boneList) {
    XMLDocument collada;
    collada.InsertFirstChild(collada.NewElement("COLLADA"));
    XMLElement *rootElement = collada.RootElement();
    rootElement->SetAttribute("version", "1.4.1");
    rootElement->SetAttribute("xmlns", "http://www.collada.org/2005/11/COLLADASchema");
    asset(collada, rootElement);
    library_geometries(collada, rootElement, meshList);
    //library_materials(collada, rootElement, meshList);
    library_controllers(collada, rootElement, meshList, boneList);
    library_visual_scenes(collada, rootElement, boneList);
    XMLPrinter printer;
    collada.Print(&printer);
    return printer.CStr();
}

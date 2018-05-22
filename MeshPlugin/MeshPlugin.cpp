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

#define SCENE_MAGIC (0x6873654D)

int createPluginInterface(PluginInterface **ppPluginInterface)
{
    if (!ppPluginInterface)
    {
        return 1;
    }
    *ppPluginInterface = new MeshPlugin;
    if (!*ppPluginInterface)
    {
        return 1;
    }
    return 0;
}

int destroyPluginInterface(PluginInterface *pPluginInterface)
{
    if (!pPluginInterface)
    {
        return 1;
    }
    delete pPluginInterface;
    return 0;
}

MeshPlugin::MeshPlugin()
{
}

MeshPlugin::~MeshPlugin()
{
}
static inline void dumpData(string fn, char *pData, size_t sz)
{
    fstream f(fn, ios_base::binary | ios_base::out);
    f.write(pData, sz);
}
//check if it's this type based on the data
int MeshPlugin::check(void *pData, size_t sz, CDRM_TYPES &type)
{
    if (sz < sizeof(fileHeader))
    {
        return 0;
    }
    fileHeader *pFileHdr = (fileHeader *)pData;
    if (pFileHdr->FILE_BYTE != 0x6 && pFileHdr->FILE_BYTE != 0x59)
    {
        return 0;
    }
    SceneHeader *pMeshHeader = pFileHdr->getSceneHeader();
    if (pMeshHeader->magic != 0x6873654D)
    {
        return 0;
    }
    type = CDRM_TYPE_MESH;
    return 1;
}

std::string get_datetime_string(std::chrono::system_clock::time_point t)
{
    char some_buffer[256] = {0};
    auto as_time_t = std::chrono::system_clock::to_time_t(t);
    struct tm tm;
    if (::gmtime_r(&as_time_t, &tm))
        if (std::strftime(some_buffer, sizeof(some_buffer), "%FT%TZ", &tm))
            return std::string{some_buffer};
    throw std::runtime_error("Failed to get current date as string");
}

int MeshPlugin::unpack(void *pDataIn, size_t szIn, void **ppDataOut, size_t &szOut, CDRM_TYPES &type)
{
    if (szIn < sizeof(fileHeader))
    {
        return 1;
    }
    fileHeader *pFileHdr = (fileHeader *)pDataIn;
    if (pFileHdr->FILE_BYTE != 0x6 && pFileHdr->FILE_BYTE != 0x59)
    {
        return 1;
    }

    SceneHeader *pSceneHeader = pFileHdr->getSceneHeader();
    if (pSceneHeader->magic != SCENE_MAGIC)
    {
        return 1;
    }
    type = CDRM_TYPE_MESH;
    m_uiControllerCount = 0;
    XMLDocument xmlScene;
    xmlScene.NewDeclaration("<?xml version=\"1.0\" encoding=\"utf - 8\"?>");
    XMLElement *collada = (XMLElement *)xmlScene.InsertFirstChild(xmlScene.NewElement("COLLADA"));
    collada->SetAttribute("xmlns", "http://www.collada.org/2005/11/COLLADASchema");
    collada->SetAttribute("version", "1.4.1");
    XMLElement *asset = (XMLElement *)collada->InsertEndChild(xmlScene.NewElement("asset"));
    XMLElement *created = (XMLElement *)asset->InsertEndChild(xmlScene.NewElement("created"));
    XMLElement *modified = (XMLElement *)asset->InsertEndChild(xmlScene.NewElement("modified"));
    //created->SetText(get_datetime_string(std::chrono::system_clock::now()));
    //modified->SetText(get_datetime_string(std::chrono::system_clock::now()));
    XMLElement *library_images = (XMLElement *)collada->InsertEndChild(xmlScene.NewElement("library_images"));
    XMLElement *library_materials = (XMLElement *)collada->InsertEndChild(xmlScene.NewElement("library_materials"));
    XMLElement *library_effects = (XMLElement *)collada->InsertEndChild(xmlScene.NewElement("library_effects"));
    XMLElement *library_geometries = (XMLElement *)collada->InsertEndChild(xmlScene.NewElement("library_geometries"));
    XMLElement *library_controllers = (XMLElement *)collada->InsertEndChild(xmlScene.NewElement("library_controllers"));
    XMLElement *library_visual_scenes = (XMLElement *)collada->InsertEndChild(xmlScene.NewElement("library_visual_scenes"));
    XMLElement *visual_scene = (XMLElement *)library_visual_scenes->InsertEndChild(xmlScene.NewElement("visual_scene"));
    visual_scene->SetAttribute("id", "RootScene");
    visual_scene->SetAttribute("name", "TRScene");
    XMLElement *scene = (XMLElement *)collada->InsertEndChild(xmlScene.NewElement("scene"));
    ((XMLElement *)scene->InsertEndChild(xmlScene.NewElement("instance_visual_scene")))->SetAttribute("url", "#RootScene");
    //	parse bones start
    BoneHeader *pBoneHdr = pFileHdr->getBoneHeader();
    if (pSceneHeader->HasBones())
    {
        Bone *pBonePtr = pFileHdr->getBoneListPtr();
        int boneCount = pFileHdr->getBoneCount();
        vector<XMLElement *> skeleton(boneCount);
        for (int boneIdx = 0; boneIdx < boneCount; boneIdx++)
        {
            char serialize[256] = {0};
            sprintf(serialize, "1.00 0.00 0.00 %.*e 0.00 1.00 0.00 %.*e 0.00 0.00 1.00 %.*e 0.00 0.00 0.00 1.00", DECIMAL_DIG, pBonePtr[boneIdx].x, DECIMAL_DIG, pBonePtr[boneIdx].y, DECIMAL_DIG, pBonePtr[boneIdx].z);
            XMLElement *parentNode = nullptr;
            if (pBonePtr[boneIdx].pid == -1)
            {
                parentNode = visual_scene;
            }
            else
            {
                parentNode = skeleton[pBonePtr[boneIdx].pid];
            }

            XMLElement *node = (XMLElement *)parentNode->InsertEndChild(xmlScene.NewElement("node"));
            node->SetAttribute("id", ("BoneNode" + to_string(boneIdx)));
            node->SetAttribute("name", ("BoneNode" + to_string(boneIdx)));
            node->SetAttribute("sid", ("Bone" + to_string(boneIdx)));
            node->SetAttribute("type", "JOINT");

            XMLElement *matrix = (XMLElement *)node->InsertEndChild(xmlScene.NewElement("matrix"));
            matrix->SetText(serialize);
            skeleton[boneIdx] = node;
        }
    }

    //	parse bones end
    int faceGroupIdx = 0;
    for (int meshIdx = 0; meshIdx < pSceneHeader->NUM_MESH; meshIdx++)
    {
        MeshHeader *pMeshHeader = pSceneHeader->getMeshHeader(meshIdx);
        const string geometryName = "Geometry" + to_string(meshIdx);

        XMLElement *geometry = (XMLElement *)library_geometries->InsertEndChild(xmlScene.NewElement("geometry"));
        geometry->SetAttribute("id", geometryName);
        geometry->SetAttribute("name", geometryName);

        XMLElement *node = (XMLElement *)visual_scene->InsertEndChild(xmlScene.NewElement("node"));
        node->SetAttribute("id", ("defaultobject" + to_string(meshIdx)));
        node->SetAttribute("name", ("defaultobject" + to_string(meshIdx)));
        node->SetAttribute("type", "NODE");
        XMLElement *matrix = (XMLElement *)node->InsertEndChild(xmlScene.NewElement("matrix"));
        matrix->SetText("1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1");
        //  if mesh doesn't have bones than instance_geometry else instance_controller
        XMLElement *technique_common = nullptr;
        if (!pSceneHeader->HasBones())
        {
            XMLElement *instance_geometry = (XMLElement *)node->InsertEndChild(xmlScene.NewElement("instance_geometry"));
            instance_geometry->SetAttribute("url", ("#Geometry" + to_string(meshIdx)));
            XMLElement *bind_material = (XMLElement *)instance_geometry->InsertEndChild(xmlScene.NewElement("bind_material"));
            technique_common = (XMLElement *)bind_material->InsertEndChild(xmlScene.NewElement("technique_common"));
        }

        XMLElement *mesh = (XMLElement *)geometry->InsertEndChild(xmlScene.NewElement("mesh"));
        XMLElement *vertices = (XMLElement *)mesh->InsertEndChild(xmlScene.NewElement("vertices"));
        vertices->SetAttribute("id", (geometryName + "-vertices"));
        struct source
        {
            string name;
            string semantic;
            int stride;
            char param[5];
        };
        const std::vector<source> sources = {{"GeomatryPos", "POSITION", 3, {'X', 'Y', 'Z'}}, {"GeomatryNorm", "NORMAL", 3, {'X', 'Y', 'Z'}}};

        XMLElement *float_array[sources.size()];
        XMLElement *source_array[sources.size()];

        for (size_t i = 0; i < sources.size(); i++)
        {
            source_array[i] = (XMLElement *)mesh->InsertFirstChild(xmlScene.NewElement("source"));
            source_array[i]->SetAttribute("id", (sources[i].name + to_string(meshIdx)));
            source_array[i]->SetAttribute("name", (sources[i].name + to_string(meshIdx)));
            float_array[i] = (XMLElement *)source_array[i]->InsertEndChild(xmlScene.NewElement("float_array"));
            float_array[i]->SetAttribute("id", (geometryName + "-array" + to_string(i)));

            XMLElement *technique_common = (XMLElement *)source_array[i]->InsertEndChild(xmlScene.NewElement("technique_common"));
            XMLElement *accessor = (XMLElement *)technique_common->InsertEndChild(xmlScene.NewElement("accessor"));
            accessor->SetAttribute("source", ("#Geometry" + to_string(meshIdx) + "-array" + to_string(i)));
            accessor->SetAttribute("stride", to_string(sources[i].stride));
            for (size_t strideIt = 0; strideIt < sources[i].stride; strideIt++)
            {
                auto param = xmlScene.NewElement("param");
                char src[2] = {0};
                src[0] = sources[i].param[strideIt];
                param->SetAttribute("name", src);
                param->SetAttribute("type", "float");
                accessor->InsertEndChild(param);
            }
            XMLElement *input = (XMLElement *)vertices->InsertEndChild(xmlScene.NewElement("input"));
            input->SetAttribute("semantic", sources[i].semantic);
            input->SetAttribute("source", ("#" + sources[i].name + to_string(meshIdx)));
        }
        XMLElement *float_array_weights = nullptr;
        XMLElement *vertex_weights = nullptr;
        //	parse bones start
        if (pSceneHeader->HasBones())
        {
            int32_t *pBoneMap = pSceneHeader->getBoneMap(meshIdx);
            XMLElement *controller = AddController(xmlScene, collada, geometry);
            {
                XMLElement *instance_controller = (XMLElement *)node->InsertEndChild(xmlScene.NewElement("instance_controller"));
                instance_controller->SetAttribute("url", "#" + string(controller->Attribute("id")));
                XMLElement *skeleton = (XMLElement *)instance_controller->InsertEndChild(xmlScene.NewElement("skeleton"));
                skeleton->SetText("#BoneNode0");
                XMLElement *bind_material = (XMLElement *)instance_controller->InsertEndChild(xmlScene.NewElement("bind_material"));
                technique_common = (XMLElement *)bind_material->InsertEndChild(xmlScene.NewElement("technique_common"));
            }
            Bone *bone = pFileHdr->getBoneListPtr();
            char sourceId[256] = {0};
            //  bone names
            sprintf(sourceId, "%s-Joints", controller->Attribute("id"));
            AddSource(xmlScene, controller->FirstChildElement("skin"), sourceId, {{"JOINT", "Name"}}, "Name_array", pBoneMap, pBoneMap + pMeshHeader->m_uiBoneMapCount,
                      [](int bonePid, char *serialize, int strSize, int &stride) {
                          sprintf(serialize, "Bone%d", bonePid);
                          stride = 1;
                      });
            //  bone transform
            sprintf(sourceId, "%s-bind_poses", controller->Attribute("id"));
            AddSource(xmlScene, controller->FirstChildElement("skin"), sourceId, {{"TRANSFORM", "float4x4"}}, "float_array", pBoneMap, pBoneMap + pMeshHeader->m_uiBoneMapCount,
                      [bone](int bonePid, char *serialize, int strSize, int &stride) {
                          sprintf(serialize, "1 0 0 %.*e 0 1 0 %.*e 0 0 1 %.*e 0 0 0 1", DECIMAL_DIG, bone[bonePid].x, DECIMAL_DIG, bone[bonePid].y, DECIMAL_DIG, bone[bonePid].z);
                          stride = 16;
                      });
            //  bone weights
            sprintf(sourceId, "%s-weights", controller->Attribute("id"));
            //  add weight source, will be populated later
            XMLElement *source = (XMLElement *)controller->FirstChildElement("skin")->InsertEndChild(xmlScene.NewElement("source"));
            source->SetAttribute("id", sourceId);

            //  bone weights array
            sprintf(sourceId, "%s-weights-array", controller->Attribute("id"));
            //  add weight source, will be populated later
            float_array_weights = (XMLElement *)source->InsertFirstChild(xmlScene.NewElement("float_array"));
            float_array_weights->SetAttribute("id", sourceId);

            //  add weight source, will be populated later
            XMLElement *technique_common = (XMLElement *)source->InsertEndChild(xmlScene.NewElement("technique_common"));
            XMLElement *accessor = (XMLElement *)technique_common->InsertEndChild(xmlScene.NewElement("accessor"));
            accessor->SetAttribute("source", string("#") + sourceId);

            XMLElement *param = (XMLElement *)accessor->InsertEndChild(xmlScene.NewElement("param"));
            param->SetAttribute("name", "WEIGHT");
            param->SetAttribute("type", "float");

            XMLElement *input = nullptr;
            XMLElement *joints = (XMLElement *)controller->FirstChildElement("skin")->InsertEndChild(xmlScene.NewElement("joints"));

            sprintf(sourceId, "%s-Joints", controller->Attribute("id"));
            input = (XMLElement *)joints->InsertEndChild(xmlScene.NewElement("input"));
            input->SetAttribute("semantic", "JOINT");
            input->SetAttribute("source", string("#") + sourceId);
            input = (XMLElement *)joints->InsertEndChild(xmlScene.NewElement("input"));
            sprintf(sourceId, "%s-bind_poses", controller->Attribute("id"));
            input->SetAttribute("semantic", "INV_BIND_MATRIX");
            input->SetAttribute("source", (string("#") + sourceId));

            vertex_weights = (XMLElement *)controller->FirstChildElement("skin")->InsertEndChild(xmlScene.NewElement("vertex_weights"));
            sprintf(sourceId, "#%s-Joints", controller->Attribute("id"));
            input = (XMLElement *)vertex_weights->InsertEndChild(xmlScene.NewElement("input"));
            input->SetAttribute("semantic", "JOINT");
            input->SetAttribute("source", sourceId);
            input->SetAttribute("offset", 0);
            sprintf(sourceId, "#%s-weights", controller->Attribute("id"));
            input = (XMLElement *)vertex_weights->InsertEndChild(xmlScene.NewElement("input"));
            input->SetAttribute("semantic", "WEIGHT");
            input->SetAttribute("source", sourceId);
            input->SetAttribute("offset", 1);
        }
        //	parse bones end

        //	unpack mesh components
        ComponentDataBlob *pComponentDataBlob = pSceneHeader->getComponentBlob(meshIdx);
        char *pComponentData = (char *)pSceneHeader->getComponentData(meshIdx);
        int strInc = 0x100000;
        size_t strSz = 0x100000;
        size_t strPos = 0;
        char *vectorString = (char *)malloc(strSz);
        if (vectorString == nullptr)
        {
            printf("Memory full\n");
            return 1;
        }
        printf("----------------------------------------------------------------\n");
        printf("Struct size: %#X\n", pComponentDataBlob->vertexSize);
        for (int compIdx = 0; compIdx < pComponentDataBlob->itemCount; compIdx++)
        {
            strPos = 0;
            ComponentValue *component = pComponentDataBlob->getComponents(compIdx);
            auto it = std::find_if(VertexTypeMap, VertexTypeMap + _countof(VertexTypeMap), [component](const auto &t) { return t.hash == component->itemOffsetType; });
            if (it == (VertexTypeMap + _countof(VertexTypeMap)))
            {
                printf("Type: %#x, Offset: %x\n", component->itemOffsetType, component->itemValue);
            }
            else
            {
                printf("Type: %s, Offset: %x\n", it->name, component->itemValue);
            }
            switch (component->itemOffsetType)
            {
            case CMP_Offset_Position:
            {
                auto position_float_array = source_array[0]->FirstChildElement("float_array");
                auto accessor = source_array[0]->FirstChildElement("technique_common")->FirstChildElement("accessor");
                position_float_array->SetAttribute("count", pMeshHeader->m_iNumVerts * sources[0].stride);
                accessor->SetAttribute("count", pMeshHeader->m_iNumVerts);

                char *pVertexDataStart = pComponentData + component->itemValue;

                for (size_t v = 0; v < pMeshHeader->m_iNumVerts; v++)
                {
                    float *pVertexPtr = (float *)(pVertexDataStart + (pComponentDataBlob->vertexSize * v));
                    if (strSz - strPos < 0x30)
                    {
                        strSz += strInc;
                        vectorString = (char *)realloc(vectorString, strSz);
                        if (vectorString == nullptr)
                        {
                            printf("Failed to increase memory\n.");
                            break;
                        }
                    }
                    size_t charWritten = snprintf(&vectorString[strPos], strSz - strPos, "%.*e %.*e %.*e ", DECIMAL_DIG, pVertexPtr[0], DECIMAL_DIG, pVertexPtr[1], DECIMAL_DIG, pVertexPtr[2]);
                    strPos += charWritten;
                }
                if (vectorString == nullptr)
                {
                    printf("Invalid Pointer\n.");
                    break;
                }
                position_float_array->SetText(vectorString);
            };
            break;
            case CMP_Offset_TexCord:
            {
                DynamicStructIterator<int16_t> tex2it((void *)pComponentData, 0, pMeshHeader->m_iNumVerts, component->itemValue, pComponentDataBlob->vertexSize);
                DynamicStructIterator<int16_t> tex2it_end((void *)pComponentData, pMeshHeader->m_iNumVerts, pMeshHeader->m_iNumVerts, component->itemValue, pComponentDataBlob->vertexSize);
                XMLElement *source = AddSource(xmlScene, mesh, "GeomatryTex0" + to_string(meshIdx), {{"S", "float"}, {"T", "float"}}, "float_array", tex2it, tex2it_end,
                                               [](const int16_t &in, char *serialize, int sz, int &stride) {
                                                   stride = 2;
                                                   float2 tmp = uvParser(&in);
                                                   snprintf(serialize, sz, "%.*e %.*e ", DECIMAL_DIG, tmp.x, DECIMAL_DIG, tmp.y);
                                               });
                XMLElement *vertices = mesh->FirstChildElement("vertices");
                if (!vertices)
                {
                    vertices = (XMLElement *)mesh->InsertEndChild(xmlScene.NewElement("vertices"));
                }
                auto input = (XMLElement *)vertices->InsertEndChild(xmlScene.NewElement("input"));
                input->SetAttribute("semantic", "TEXCOORD");
                input->SetAttribute("source", "#" + string(source->Attribute("id")));
                input->SetAttribute("set", "0");
            }
            break;
            case CMP_Offset_TexCord2:
            {
                DynamicStructIterator<int16_t> tex2it((void *)pComponentData, 0, pMeshHeader->m_iNumVerts, component->itemValue, pComponentDataBlob->vertexSize);
                DynamicStructIterator<int16_t> tex2it_end((void *)pComponentData, pMeshHeader->m_iNumVerts, pMeshHeader->m_iNumVerts, component->itemValue, pComponentDataBlob->vertexSize);
                XMLElement *source = AddSource(xmlScene, mesh, "GeomatryTex1" + to_string(meshIdx), {{"S", "float"}, {"T", "float"}}, "float_array", tex2it, tex2it_end,
                                               [](const int16_t &in, char *serialize, int sz, int &stride) {
                                                   stride = 2;
                                                   float2 tmp = uvParser(&in);
                                                   snprintf(serialize, sz, "%.*e %.*e ", DECIMAL_DIG, tmp.x, DECIMAL_DIG, tmp.y);
                                               });
                XMLElement *vertices = mesh->FirstChildElement("vertices");
                if (!vertices)
                {
                    vertices = (XMLElement *)mesh->InsertEndChild(xmlScene.NewElement("vertices"));
                }
                auto input = (XMLElement *)vertices->InsertEndChild(xmlScene.NewElement("input"));
                input->SetAttribute("semantic", "TEXCOORD");
                input->SetAttribute("source", "#" + string(source->Attribute("id")));
                input->SetAttribute("set", "1");
            }
            break;
            case CMP_Offset_BoneWeight:
            {
                auto accessor = float_array_weights->NextSiblingElement("technique_common")->FirstChildElement("accessor");
                float_array_weights->SetAttribute("count", pMeshHeader->m_iNumVerts * 4);
                accessor->SetAttribute("count", pMeshHeader->m_iNumVerts * 4);

                char *pBoneDataStart = pComponentData + component->itemValue;
                for (size_t v = 0; v < pMeshHeader->m_iNumVerts; v++)
                {
                    uint8_t *pBonePtr = (uint8_t *)(pBoneDataStart + (pComponentDataBlob->vertexSize * v));
                    float4 tmp = boneParser(pBonePtr);
                    if (strSz - strPos < 0x30)
                    {
                        strSz += strInc;
                        vectorString = (char *)realloc(vectorString, strSz);
                        if (vectorString == nullptr)
                        {
                            printf("Failed to increase memory\n.");
                            break;
                        }
                    }
                    size_t charWritten = snprintf(&vectorString[strPos], strSz - strPos, "%.*e %.*e %.*e %.*e ", DECIMAL_DIG, tmp.x, DECIMAL_DIG, tmp.y, DECIMAL_DIG, tmp.z, DECIMAL_DIG, tmp.w);
                    strPos += charWritten;
                }
                if (vectorString == nullptr)
                {
                    printf("Invalid Pointer\n.");
                    break;
                }
                float_array_weights->SetText(vectorString);
            }
            break;
            case CMP_Offset_BoneIndex:
            {
                XMLElement *vcount = (XMLElement *)vertex_weights->InsertEndChild(xmlScene.NewElement("vcount"));
                {
                    vector<char[2]> tmpData(pMeshHeader->m_iNumVerts);
                    for_each(tmpData.begin(), tmpData.end(), [](auto &a) {a[0]='4';a[1]=' '; });
                    tmpData.back()[1] = 0;
                    vcount->SetText((char *)tmpData.data());
                }
                XMLElement *v = (XMLElement *)vertex_weights->InsertEndChild(xmlScene.NewElement("v"));
                vertex_weights->SetAttribute("count", pMeshHeader->m_iNumVerts); // each vertex has 4 weights

                char *pBoneIDXDataStart = pComponentData + component->itemValue;
                for (int v = 0; v < pMeshHeader->m_iNumVerts; v++)
                {
                    uint8_t *pBoneIDXPtr = (uint8_t *)(pBoneIDXDataStart + (pComponentDataBlob->vertexSize * v));
                    int4 tmp = boneMapParser(pBoneIDXPtr);
                    if (strSz - strPos < 0x30)
                    {
                        strSz += strInc;
                        vectorString = (char *)realloc(vectorString, strSz);
                        if (vectorString == nullptr)
                        {
                            printf("Failed to increase memory\n.");
                            break;
                        }
                    }
                    size_t charWritten = snprintf(&vectorString[strPos], strSz - strPos, "%d %d %d %d %d %d %d %d ", tmp.x, (v * 4) + 0, tmp.y, (v * 4) + 1, tmp.z, (v * 4) + 2, tmp.w, (v * 4) + 3);
                    strPos += charWritten;
                }
                if (vectorString == nullptr)
                {
                    printf("Invalid Pointer\n.");
                    break;
                }
                v->SetText(vectorString);
            }
            break;
            case CMP_Offset_UnknownMatrix:
                break;
            case CMP_Offset_Normal:
            {
                auto normal_float_array = source_array[1]->FirstChildElement("float_array");
                auto accessor = source_array[1]->FirstChildElement("technique_common")->FirstChildElement("accessor");
                normal_float_array->SetAttribute("count", pMeshHeader->m_iNumVerts * 3);
                accessor->SetAttribute("count", pMeshHeader->m_iNumVerts);

                char *pNormDataStart = pComponentData + component->itemValue;
                for (size_t v = 0; v < pMeshHeader->m_iNumVerts; v++)
                {
                    uint8_t *pNormPtr = (uint8_t *)(pNormDataStart + (pComponentDataBlob->vertexSize * v));
                    float3 tmp = normParser(pNormPtr);
                    if (strSz - strPos < 0x30)
                    {
                        strSz += strInc;
                        vectorString = (char *)realloc(vectorString, strSz);
                        if (vectorString == nullptr)
                        {
                            printf("Failed to increase memory\n.");
                            break;
                        }
                    }
                    size_t charWritten = snprintf(&vectorString[strPos], strSz - strPos, "%.*e %.*e %.*e ", DECIMAL_DIG, tmp.x, DECIMAL_DIG, tmp.y, DECIMAL_DIG, tmp.z);
                    strPos += charWritten;
                }
                if (vectorString == nullptr)
                {
                    printf("Invalid Pointer\n.");
                    break;
                }
                normal_float_array->SetText(vectorString);
            }
            break;
            default:
                //printf("Unknown offset type [%#x]\n", component->itemOffsetType);
                break;
            }
        }
        free(vectorString);
        for (int faceIdx = 0; faceIdx < pMeshHeader->m_iNumIndexGroups; faceIdx++)
        {
            FaceHeader *pFaceHeader = pSceneHeader->getFaceHeader(faceGroupIdx);
            uint16_t *pIndexBuffer = pSceneHeader->getIndexBuffer(faceGroupIdx);

            // create material if needed
            AddMaterial(xmlScene, collada, "Material_" + to_string(pFaceHeader->matNum), "Texture_" + to_string(pFaceHeader->matNum));

            XMLElement *instance_material = (XMLElement *)technique_common->InsertEndChild(xmlScene.NewElement("instance_material"));
            instance_material->SetAttribute("symbol", ("Material_" + to_string(pFaceHeader->matNum)));
            instance_material->SetAttribute("target", ("#Material_" + to_string(pFaceHeader->matNum)));

            XMLElement *triangles = (XMLElement *)mesh->InsertEndChild(xmlScene.NewElement("triangles"));
            triangles->SetAttribute("count", to_string(pFaceHeader->numFaces));
            triangles->SetAttribute("material", ("Material_" + to_string(pFaceHeader->matNum)));

            XMLElement *input = (XMLElement *)triangles->InsertEndChild(xmlScene.NewElement("input"));
            input->SetAttribute("semantic", "VERTEX");
            input->SetAttribute("source", ("#Geometry" + to_string(meshIdx) + "-vertices"));
            input->SetAttribute("offset", "0");
            XMLElement *p = (XMLElement *)triangles->InsertEndChild(xmlScene.NewElement("p"));
            stringstream indexBufferStream;
            for (size_t idx = 0; idx < pFaceHeader->numFaces * 3; idx++)
            {
                indexBufferStream << pIndexBuffer[idx] << " ";
            }
            p->SetText(indexBufferStream.str());
            faceGroupIdx++;
        }
    }

    XMLPrinter xmlPrinter;
    xmlScene.Accept(&xmlPrinter);
    szOut = xmlPrinter.CStrSize();
    *ppDataOut = new char[szOut];
    strcpy((char *)*ppDataOut, xmlPrinter.CStr());
    szOut = strlen((char *)*ppDataOut);
    return 0;
}

int MeshPlugin::pack(void *buffer, size_t sz, void **data, size_t &sz_data, CDRM_TYPES &type)
{
    const aiScene *scene = nullptr;
    Assimp::Importer importer;
    scene = importer.ReadFileFromMemory(buffer, sz, 0);
    for (int m = 0; m < scene->mNumMeshes; m++)
    {
        printf("%d. Mesh [%s], no. UV channels [%d], no. bones [%d]\n", m, scene->mMeshes[m]->mName.C_Str(), scene->mMeshes[m]->GetNumUVChannels(), scene->mMeshes[m]->mNumBones);
    }
    if (!scene)
    {
        printf("Invalid scene\n");
    }
    printf("Not implimented\n");
    return 1;
}

int MeshPlugin::getType(void *, size_t)
{
    return 1;
}

float2 MeshPlugin::uvParser(const int16_t *pTex)
{
    float tmp = 0;
    float u = modf((float)pTex[0] / 2048.0f, &tmp);
    float v = modf((float)pTex[1] / 2048.0f, &tmp);
    return float2(u, 1.0f - v);
}

float3 MeshPlugin::normParser(uint8_t *pNorm)
{
    float3 normFlt((255.0f * (float)pNorm[0]) - (128.0f - (float)pNorm[3]),
                   (255.0f * (float)pNorm[1]) - (128.0f - (float)pNorm[3]),
                   -((255.0f * (float)pNorm[2]) - (128.0f - (float)pNorm[3])));
    return normFlt.Normalize();
}

float4 MeshPlugin::boneParser(uint8_t *pBone)
{
    return float4((float)pBone[0] / 255.0, (float)pBone[1] / 255.0, (float)pBone[2] / 255.0, (float)pBone[3] / 255.0);
}

int4 MeshPlugin::boneMapParser(uint8_t *pBoneMap)
{
    return int4((float)pBoneMap[0], (float)pBoneMap[1], (float)pBoneMap[2], (float)pBoneMap[3]);
}

XMLElement *MeshPlugin::AddMaterial(XMLDocument &xmlScene, XMLElement *scene, string matName, string diffTexName)
{
    XMLElement *library_materials = scene->FirstChildElement("library_materials");
    if (!library_materials)
    {
        library_materials = (XMLElement *)scene->InsertEndChild(xmlScene.NewElement("library_materials"));
    }
    XMLElement *material = nullptr;
    for (material = library_materials->FirstChildElement("material");
         material && !material->Attribute("id", matName);
         material = material->NextSiblingElement("material"))
        ;
    if (!material)
    {
        material = (XMLElement *)library_materials->InsertEndChild(xmlScene.NewElement("material"));
        material->SetAttribute("id", matName);

        XMLElement *effect = AddEffect(xmlScene, scene, matName + "-fx", "phong", diffTexName);
        XMLElement *instance_effect = (XMLElement *)material->InsertEndChild(xmlScene.NewElement("instance_effect"));
        instance_effect->SetAttribute("url", ("#" + matName + "-fx"));
    }
    return material;
}

XMLElement *MeshPlugin::AddTexture(XMLDocument &xmlScene, XMLElement *scene, string diffTexName)
{
    XMLElement *library_images = scene->FirstChildElement("library_images");
    if (!library_images)
    {
        library_images = (XMLElement *)scene->InsertEndChild(xmlScene.NewElement("library_images"));
    }
    XMLElement *image = nullptr;
    for (image = library_images->FirstChildElement("image");
         image && !image->Attribute("id", diffTexName);
         image = image->NextSiblingElement("image"))
        ;
    if (!image)
    {
        image = (XMLElement *)library_images->InsertEndChild(xmlScene.NewElement("image"));
        image->SetAttribute("id", diffTexName);

        XMLElement *init_from = (XMLElement *)image->InsertEndChild(xmlScene.NewElement("init_from"));
        init_from->SetText((diffTexName + ".dds"));
    }
    return image;
}

XMLElement *MeshPlugin::AddEffect(XMLDocument &xmlScene, XMLElement *scene, string effectName, string techniqueName, string diffTexName)
{
    XMLElement *library_effects = scene->FirstChildElement("library_effects");
    if (!library_effects)
    {
        library_effects = (XMLElement *)scene->InsertEndChild(xmlScene.NewElement("library_effects"));
    }
    XMLElement *effect = nullptr;
    for (effect = library_effects->FirstChildElement("effect");
         effect && !effect->Attribute("id", effectName);
         effect = effect->NextSiblingElement("effect"))
        ;

    if (!effect)
    {
        effect = (XMLElement *)library_effects->InsertEndChild(xmlScene.NewElement("effect"));
        effect->SetAttribute("id", effectName);

        auto profile_COMMON = (XMLElement *)effect->InsertEndChild(xmlScene.NewElement("profile_COMMON"));
        XMLElement *texture = AddTexture(xmlScene, scene, diffTexName);

        //	add surface
        auto newparam = (XMLElement *)profile_COMMON->InsertEndChild(xmlScene.NewElement("newparam"));
        newparam->SetAttribute("sid", (diffTexName + "-surface"));
        auto surface = (XMLElement *)newparam->InsertEndChild(xmlScene.NewElement("surface"));
        surface->SetAttribute("type", "2D");
        auto init_from = (XMLElement *)surface->InsertEndChild(xmlScene.NewElement("init_from"));
        init_from->SetText(diffTexName);
        auto format = (XMLElement *)surface->InsertEndChild(xmlScene.NewElement("format"));
        format->SetText("A8R8G8B8");

        //	add sampler
        auto newparam1 = (XMLElement *)profile_COMMON->InsertEndChild(xmlScene.NewElement("newparam"));
        newparam1->SetAttribute("sid", (diffTexName + "-sampler"));
        auto sampler2D = (XMLElement *)newparam1->InsertEndChild(xmlScene.NewElement("sampler2D"));
        auto source = (XMLElement *)sampler2D->InsertEndChild(xmlScene.NewElement("source"));
        source->SetText((diffTexName + "-surface"));
        auto wrap_s = (XMLElement *)sampler2D->InsertEndChild(xmlScene.NewElement("wrap_s"));
        wrap_s->SetText("WRAP");
        auto wrap_t = (XMLElement *)sampler2D->InsertEndChild(xmlScene.NewElement("wrap_t"));
        wrap_t->SetText("WRAP");
        auto minfilter = (XMLElement *)sampler2D->InsertEndChild(xmlScene.NewElement("minfilter"));
        minfilter->SetText("NONE");
        auto magfilter = (XMLElement *)sampler2D->InsertEndChild(xmlScene.NewElement("magfilter"));
        magfilter->SetText("NONE");
        auto mipfilter = (XMLElement *)sampler2D->InsertEndChild(xmlScene.NewElement("mipfilter"));
        mipfilter->SetText("NONE");

        auto technique = (XMLElement *)profile_COMMON->InsertEndChild(xmlScene.NewElement("technique"));
        technique->SetAttribute("sid", "standard");
        auto phong = (XMLElement *)technique->InsertEndChild(xmlScene.NewElement("phong"));

        auto diffuse = (XMLElement *)phong->InsertEndChild(xmlScene.NewElement("diffuse"));
        auto texture1 = (XMLElement *)diffuse->InsertEndChild(xmlScene.NewElement("texture"));
        texture1->SetAttribute("texture", (diffTexName + "-sampler"));
        texture1->SetAttribute("texcoord", "TEXCOORD");
    }
    return effect;
}

XMLElement *MeshPlugin::AddController(XMLDocument &xmlScene, XMLElement *scene, XMLElement *mesh)
{
    XMLElement *library_controllers = scene->FirstChildElement("library_controllers");
    if (!library_controllers)
    {
        library_controllers = (XMLElement *)scene->InsertEndChild(xmlScene.NewElement("library_effects"));
    }
    XMLElement *controller = nullptr;
    string controllerName = "Controller" + to_string(m_uiControllerCount);
    for (controller = library_controllers->FirstChildElement("controller");
         controller && !controller->Attribute("id", controllerName);
         controller = controller->NextSiblingElement("controller"))
        ;

    if (!controller)
    {
        controller = (XMLElement *)library_controllers->InsertEndChild(xmlScene.NewElement("controller"));
        controller->SetAttribute("id", controllerName);

        m_uiControllerCount++;
        auto skin = (XMLElement *)controller->InsertEndChild(xmlScene.NewElement("skin"));
        skin->SetAttribute("source", (string("#") + mesh->Attribute("name")));
        //XMLElement* texture = AddTexture(xmlScene, scene, diffTexName);

        //	bind_shape_matrix
        auto bind_shape_matrix = (XMLElement *)skin->InsertEndChild(xmlScene.NewElement("bind_shape_matrix"));
        bind_shape_matrix->SetText("1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1");
    }
    return controller;
}

template <class InpTypeIt, class fnType>
XMLElement *MeshPlugin::AddSource(XMLDocument &xmlScene, XMLElement *root, const string &id, const vector<MeshPlugin::AccessorType> &accessors, const string &array_type_name, InpTypeIt start, InpTypeIt end, fnType fnGetVal)
{
    if (!root)
    {
        return nullptr;
    }
    XMLElement *source = (XMLElement *)root->InsertEndChild(xmlScene.NewElement("source"));
    source->SetAttribute("id", id);
    //source->SetAttribute("name", (sources[i].name + to_string(meshIdx)));
    XMLElement *array_type = (XMLElement *)source->InsertEndChild(xmlScene.NewElement(array_type_name));
    array_type->SetAttribute("id", (id + "-array"));
    XMLElement *technique_common = (XMLElement *)source->InsertEndChild(xmlScene.NewElement("technique_common"));

    XMLElement *accessor = (XMLElement *)technique_common->InsertEndChild(xmlScene.NewElement("accessor"));
    accessor->SetAttribute("source", ("#" + id + "-array"));
    for (const auto &parameter : accessors)
    {
        XMLElement *param = (XMLElement *)accessor->InsertEndChild(xmlScene.NewElement("param"));
        param->SetAttribute("name", parameter.name);
        param->SetAttribute("type", parameter.type);
    }
    int count = 0;
    int strided_count = 0;
    stringstream source_array;
    int stride = 0;
    char serialize[256];
    for (auto it = start; it != end; ++it)
    {
        fnGetVal(*it, serialize, sizeof(serialize) / sizeof(serialize[0]), stride);
        source_array << serialize << " ";
        count += stride;
        strided_count++;
    }
    array_type->SetText(source_array.str());
    array_type->SetAttribute("count", count);
    accessor->SetAttribute("count", strided_count);
    accessor->SetAttribute("stride", stride);
    return source;
}

BoneHeader *fileHeader::getBoneHeader()
{
    return (BoneHeader *)((char *)this + (FILE_BYTE * 8));
}

SceneHeader *fileHeader::getSceneHeader()
{
    return (SceneHeader *)((char *)this + (FILE_BYTE * 8) + 20 + (NR_OF_MESHES * 4) + PTR_MESH_START);
}

size_t fileHeader::getMeshHeaderOffset()
{
    return ((size_t)FILE_BYTE * 8) + 20 + ((size_t)NR_OF_MESHES * 4) + (size_t)PTR_MESH_START;
}

int32_t fileHeader::getBoneCount()
{
    return *(int32_t *)((char *)this + (FILE_BYTE * 8) + 20 + (NR_OF_MESHES * 4) + getBoneHeader()->getBoneCountOffset());
}

Bone *fileHeader::getBoneListPtr()
{
    return (Bone *)((char *)this + (FILE_BYTE * 8) + 20 + (NR_OF_MESHES * 4) + getBoneHeader()->getBoneStartOffset());
}

MeshHeader *SceneHeader::getMeshHeader(int meshIdx)
{
    return (MeshHeader *)(((char *)this + PTR_MESH_TBL00) + (meshIdx * sizeof(MeshHeader)));
}

int32_t *SceneHeader::getBoneMap(int meshIdx)
{
    return (int32_t *)((char *)this + getMeshHeader(meshIdx)->BoneMapOffset);
}

ComponentDataBlob *SceneHeader::getComponentBlob(int meshIdx)
{
    return (ComponentDataBlob *)((char *)this + getMeshHeader(meshIdx)->vfOffset);
}

void *SceneHeader::getComponentData(int meshIdx)
{
    return (void *)((char *)this + getMeshHeader(meshIdx)->vertOffset);
}

FaceHeader *SceneHeader::getFaceHeader(int faceIdx)
{
    return (FaceHeader *)((char *)this + PTR_MAT_TABLE + (faceIdx * sizeof(FaceHeader)));
}

uint16_t *SceneHeader::getIndexBuffer(int faceIdx)
{
    return (uint16_t *)((char *)this + PTR_FACES_TABLE + getFaceHeader(faceIdx)->getIndexBufferOffset());
}

bool SceneHeader::HasBones()
{
    return (NUM_BONES > 0);
}

ComponentValue *ComponentDataBlob::getComponents(int componentIdx)
{
    return ((ComponentValue *)&this[1]) + componentIdx;
}

size_t FaceHeader::getIndexBufferOffset()
{
    return indexFrom * sizeof(uint16_t);
}

uint32_t BoneHeader::getBoneCountOffset()
{
    return offsetIBoneCount - 4;
}

uint32_t BoneHeader::getBoneStartOffset()
{
    return offsetIBoneStart;
}

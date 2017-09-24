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
#include "MeshPlugin.h"
#include <string>
#include <fstream>
#include <vector>
#include <DirectXMath.h>
#include <sstream>
#include <algorithm>
#include <functional>


int createPluginInterface(PluginInterface ** ppPluginInterface)
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

int destroyPluginInterface(PluginInterface * pPluginInterface)
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
static inline void dumpData(string fn, char* pData, size_t sz)
{
    fstream f(fn, ios_base::binary | ios_base::out);
    f.write(pData, sz);
}
//check if it's this type based on the data
int MeshPlugin::check(void *pData, size_t sz, CDRM_TYPES & type)
{
    if (sz < sizeof(fileHeader))
    {
        return 0;
    }
    fileHeader *pFileHdr = (fileHeader*)pData;
    if (pFileHdr->FILE_BYTE != 0x6 && pFileHdr->FILE_BYTE != 0x59)
    {
        return 0;
    }
    //dumpData("mesh.mesh", (char*)pData, sz);
    SceneHeader* pMeshHeader = pFileHdr->getSceneHeader();
    if (pMeshHeader->magic != 0x6873654D)
    {
        return 0;
    }
    type = CDRM_TYPE_MESH;
    return 1;
}

int MeshPlugin::unpack(void *pDataIn, size_t szIn, void **ppDataOut, size_t &szOut, CDRM_TYPES &type)
{
    if (szIn < sizeof(fileHeader))
    {
        return 1;
    }
    fileHeader *pFileHdr = (fileHeader*)pDataIn;
    if (pFileHdr->FILE_BYTE != 0x6 && pFileHdr->FILE_BYTE != 0x59)
    {
        return 1;
    }

    SceneHeader* pSceneHeader = pFileHdr->getSceneHeader();
    if (pSceneHeader->magic != 0x6873654D)
    {
        return 1;
    }
    type = CDRM_TYPE_MESH;
    XMLDocument xmlScene;
    xmlScene.NewDeclaration("<?xml version=\"1.0\" encoding=\"utf - 8\"?>");
    XMLElement* collada = (XMLElement*)xmlScene.InsertFirstChild(xmlScene.NewElement("COLLADA"));
    collada->SetAttribute("xmlns", "http://www.collada.org/2005/11/COLLADASchema");
    collada->SetAttribute("version", "1.4.1");
    auto asset = collada->InsertEndChild(xmlScene.NewElement("asset"));
    XMLElement* library_images = (XMLElement*)collada->InsertEndChild(xmlScene.NewElement("library_images"));
    XMLElement* library_effects = (XMLElement*)collada->InsertEndChild(xmlScene.NewElement("library_effects"));
    XMLElement* library_geometries = (XMLElement*)collada->InsertEndChild(xmlScene.NewElement("library_geometries"));
    XMLElement* library_controllers = (XMLElement*)collada->InsertEndChild(xmlScene.NewElement("library_controllers"));
    XMLElement* library_visual_scenes = (XMLElement*)collada->InsertEndChild(xmlScene.NewElement("library_visual_scenes"));
    XMLElement* visual_scene = (XMLElement*)library_visual_scenes->InsertEndChild(xmlScene.NewElement("visual_scene"));
    visual_scene->SetAttribute("id", "RootScene");
    visual_scene->SetAttribute("name", "TRScene");
    XMLElement* scene = (XMLElement*)collada->InsertEndChild(xmlScene.NewElement("scene"));
    ((XMLElement*)scene->InsertEndChild(xmlScene.NewElement("instance_visual_scene")))->SetAttribute("url", "#RootScene");


    //	parse bones start
    //	parse bones end
    int faceGroupIdx = 0;
    for (int meshIdx = 0; meshIdx < pSceneHeader->NUM_MESH; meshIdx++)
    {
        vector<int> vFaceList;
        vector<XMFLOAT3> vVertexList;
        vector<XMFLOAT2> vTexCordList;
        vector<XMFLOAT3> vNormList;
        MeshHeader *pMeshHeader = pSceneHeader->getMeshHeader(meshIdx);

        XMLElement* geometry = (XMLElement*)library_geometries->InsertEndChild(xmlScene.NewElement("geometry"));
        geometry->SetAttribute("id", ("Geometry" + to_string(meshIdx)).c_str());
        geometry->SetAttribute("name", ("Geometry" + to_string(meshIdx)).c_str());

        XMLElement* node = (XMLElement*)visual_scene->InsertEndChild(xmlScene.NewElement("node"));
        node->SetAttribute("id", ("defaultobject" + to_string(meshIdx)).c_str());
        node->SetAttribute("name", ("defaultobject" + to_string(meshIdx)).c_str());
        XMLElement* matrix = (XMLElement*)node->InsertEndChild(xmlScene.NewElement("matrix"));
        matrix->SetText("1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1");
        XMLElement* instance_geometry = (XMLElement*)node->InsertEndChild(xmlScene.NewElement("instance_geometry"));
        instance_geometry->SetAttribute("url", ("#Geometry" + to_string(meshIdx)).c_str());

        XMLElement* bind_material = (XMLElement*)instance_geometry->InsertEndChild(xmlScene.NewElement("bind_material"));
        XMLElement* technique_common = (XMLElement*)bind_material->InsertEndChild(xmlScene.NewElement("technique_common"));

        XMLElement* mesh = (XMLElement*)geometry->InsertEndChild(xmlScene.NewElement("mesh"));
        XMLElement* vertices = (XMLElement*)mesh->InsertEndChild(xmlScene.NewElement("vertices"));
        vertices->SetAttribute("id", ("Geometry" + to_string(meshIdx) + "-vertices").c_str());
        struct source
        {
            string name;
            string semantic;
            int stride;
            char param[5];
        };
        source sources[] = { { "GeomatryPos","POSITION",3,{'X','Y','Z'} },{ "GeomatryNorm","NORMAL",3,{ 'X','Y','Z' } },{ "GeomatryTex","TEXCOORD",2,{ 'S','T' } } };
        XMLElement* float_array[_countof(sources)] = { nullptr };
        XMLElement* source_array[_countof(sources)] = { nullptr };

        for (size_t i = 0; i < _countof(sources); i++)
        {
            source_array[i] = (XMLElement*)mesh->InsertFirstChild(xmlScene.NewElement("source"));
            source_array[i]->SetAttribute("id", (sources[i].name + to_string(meshIdx)).c_str());
            source_array[i]->SetAttribute("name", (sources[i].name + to_string(meshIdx)).c_str());
            float_array[i] = (XMLElement*)source_array[i]->InsertEndChild(xmlScene.NewElement("float_array"));
            float_array[i]->SetAttribute("id", ("Geometry" + to_string(meshIdx) + "-array" + to_string(i)).c_str());

            XMLElement* technique_common = (XMLElement*)source_array[i]->InsertEndChild(xmlScene.NewElement("technique_common"));
            XMLElement* accessor = (XMLElement*)technique_common->InsertEndChild(xmlScene.NewElement("accessor"));
            accessor->SetAttribute("source", ("#Geometry" + to_string(meshIdx) + "-array" + to_string(i)).c_str());
            accessor->SetAttribute("stride", to_string(sources[i].stride).c_str());
            for (size_t strideIt = 0; strideIt < sources[i].stride; strideIt++)
            {
                auto param = xmlScene.NewElement("param");
                char src[2] = { 0 };
                src[0] = sources[i].param[strideIt];
                param->SetAttribute("name", src);
                param->SetAttribute("type", "float");
                accessor->InsertEndChild(param);
            }
            XMLElement* input = (XMLElement*)vertices->InsertEndChild(xmlScene.NewElement("input"));
            input->SetAttribute("semantic", sources[i].semantic.c_str());
            input->SetAttribute("source", ("#" + sources[i].name + to_string(meshIdx)).c_str());

        }

        //	parse bones start
        //	parse bones end


        //	unpack mesh components
        ComponentDataBlob *pComponentDataBlob = pSceneHeader->getComponentBlob(meshIdx);
        char* pComponentData = (char*)pSceneHeader->getComponentData(meshIdx);
        int strInc = 0x100000;
        size_t strSz = 0x100000;
        size_t strPos = 0;
        char *vectorString = (char*)malloc(strSz);
        
        for (int compIdx = 0; compIdx < pComponentDataBlob->itemCount; compIdx++)
        {
            strPos = 0;            
            ComponentValue *component = pComponentDataBlob->getComponents(compIdx);
            switch (component->itemOffsetType)
            {
            case CMP_Offset_Position:
            {
                auto position_float_array = source_array[0]->FirstChildElement("float_array");
                auto accessor = source_array[0]->FirstChildElement("technique_common")->FirstChildElement("accessor");
                position_float_array->SetAttribute("count", pMeshHeader->numVerts * sources[0].stride);
                accessor->SetAttribute("count", pMeshHeader->numVerts);

                char* pVertexDataStart = pComponentData + component->itemValue;
                for (size_t v = 0; v < pMeshHeader->numVerts; v++)
                {
                    float* pVertexPtr = (float*)(pVertexDataStart + (pComponentDataBlob->vertexSize*v));
                    vVertexList.push_back(XMFLOAT3(pVertexPtr));
                    if (strSz - strPos < 0x30)
                    {
                        strSz += strInc;
                        vectorString = (char*)realloc(vectorString, strSz);
                    }
                    size_t charWritten=snprintf(&vectorString[strPos], strSz - strPos, "%f %f %f ", pVertexPtr[0], pVertexPtr[1], pVertexPtr[2]);
                    strPos += charWritten;
                }
                position_float_array->SetText(vectorString);
            };
            break;
            case CMP_Offset_TexCord:
            {
                auto texture_float_array = source_array[2]->FirstChildElement("float_array");
                auto accessor = source_array[2]->FirstChildElement("technique_common")->FirstChildElement("accessor");
                texture_float_array->SetAttribute("count", pMeshHeader->numVerts * sources[2].stride);
                accessor->SetAttribute("count", pMeshHeader->numVerts);

                char* pTexCordDataStart = pComponentData + component->itemValue;
                for (size_t v = 0; v < pMeshHeader->numVerts; v++)
                {
                    int16_t* pTexCordPtr = (int16_t*)(pTexCordDataStart + (pComponentDataBlob->vertexSize*v));
                    XMFLOAT2 tmp = uvParser(pTexCordPtr);
                    vTexCordList.push_back(tmp);
                    if (strSz - strPos < 0x30)
                    {
                        strSz += strInc;
                        vectorString = (char*)realloc(vectorString, strSz);
                    }
                    size_t charWritten = snprintf(&vectorString[strPos], strSz - strPos, "%f %f ", tmp.x, tmp.y);
                    strPos += charWritten;
                }
                texture_float_array->SetText(vectorString);
            }
            break;
            case CMP_Offset_TexCord2:
                break;
            case CMP_Offset_BoneWeight:
                break;
            case CMP_Offset_BoneIndex:
                break;
            case CMP_Offset_Unknown:
                break;
            case CMP_Offset_Normal:
            {
                auto normal_float_array = source_array[1]->FirstChildElement("float_array");
                auto accessor = source_array[1]->FirstChildElement("technique_common")->FirstChildElement("accessor");
                normal_float_array->SetAttribute("count", pMeshHeader->numVerts * 3);
                accessor->SetAttribute("count", pMeshHeader->numVerts);

                char* pNormDataStart = pComponentData + component->itemValue;
                for (size_t v = 0; v < pMeshHeader->numVerts; v++)
                {
                    uint8_t* pNormPtr = (uint8_t*)(pNormDataStart + (pComponentDataBlob->vertexSize*v));
                    XMFLOAT3 tmp = normParser(pNormPtr);
                    vNormList.push_back(tmp);
                    if (strSz - strPos < 0x30)
                    {
                        strSz += strInc;
                        vectorString = (char*)realloc(vectorString, strSz);
                    }
                    size_t charWritten = snprintf(&vectorString[strPos], strSz - strPos, "%f %f %f ", tmp.x, tmp.y , tmp.z);
                    strPos += charWritten;
                }
                normal_float_array->SetText(vectorString);
            }
            break;
            default:
                printf("Unknown offset type [%#x]\n", component->itemOffsetType);
                break;
            }
        }
        free(vectorString);
        for (int faceIdx = 0; faceIdx < pMeshHeader->numFaceGroups; faceIdx++)
        {
            FaceHeader* pFaceHeader = pSceneHeader->getFaceHeader(faceGroupIdx);
            uint16_t* pIndexBuffer = pSceneHeader->getIndexBuffer(faceGroupIdx);

            // create material if needed
            AddMaterial(xmlScene, collada, "Material_" + to_string(pFaceHeader->matNum), "Texture_" + to_string(pFaceHeader->matNum));

            XMLElement* instance_material = (XMLElement*)technique_common->InsertEndChild(xmlScene.NewElement("instance_material"));
            instance_material->SetAttribute("symbol", ("Material_" + to_string(pFaceHeader->matNum)).c_str());
            instance_material->SetAttribute("target", ("#Material_" + to_string(pFaceHeader->matNum)).c_str());

            XMLElement* triangles = (XMLElement*)mesh->InsertEndChild(xmlScene.NewElement("triangles"));
            triangles->SetAttribute("count", to_string(pFaceHeader->numFaces).c_str());
            triangles->SetAttribute("material", ("Material_" + to_string(pFaceHeader->matNum)).c_str());

            XMLElement* input = (XMLElement*)triangles->InsertEndChild(xmlScene.NewElement("input"));
            input->SetAttribute("semantic", "VERTEX");
            input->SetAttribute("source", ("#Geometry" + to_string(meshIdx) + "-vertices").c_str());
            input->SetAttribute("offset", "0");
            XMLElement* p = (XMLElement*)triangles->InsertEndChild(xmlScene.NewElement("p"));
            stringstream indexBufferStream;
            for (size_t idx = 0; idx < pFaceHeader->numFaces * 3; idx++)
            {
                indexBufferStream << pIndexBuffer[idx] << " ";
            }
            p->SetText(indexBufferStream.str().c_str());
            faceGroupIdx++;
        }
    }

    XMLPrinter xmlPrinter;
    xmlScene.Accept(&xmlPrinter);
    szOut = xmlPrinter.CStrSize();
    *ppDataOut = new char[szOut];
    strcpy_s((char*)*ppDataOut, szOut, xmlPrinter.CStr());

    return 0;
}

int MeshPlugin::pack(void *, size_t, void **, size_t &, CDRM_TYPES &)
{
    printf("Not implimented");
    return 1;
}

int MeshPlugin::getType(void *, size_t)
{
    return 1;
}

XMFLOAT2 MeshPlugin::uvParser(int16_t * pTex)
{
    float tmp = 0;
    float u = modf((float)pTex[0] / 2048.0f, &tmp);
    float v = modf((float)pTex[1] / 2048.0f, &tmp);
    return XMFLOAT2(u, 1.0f-v);
}

XMFLOAT3 MeshPlugin::normParser(uint8_t * pNorm)
{
    XMFLOAT3 normFlt((255.0f * (float)pNorm[0]) - (128.0f - (float)pNorm[3]),
        (255.0f * (float)pNorm[1]) - (128.0f - (float)pNorm[3]),
        -((255.0f * (float)pNorm[2]) - (128.0f - (float)pNorm[3])));
    XMVECTOR normVec = XMLoadFloat3(&normFlt);
    XMStoreFloat3(&normFlt, XMVector3Normalize(normVec));
    return normFlt;
}

XMLElement * MeshPlugin::AddMaterial(XMLDocument &xmlScene, XMLElement * scene, string matName, string diffTexName)
{
    XMLElement* library_materials = scene->FirstChildElement("library_materials");
    if (!library_materials)
    {
        library_materials = (XMLElement*)scene->InsertEndChild(xmlScene.NewElement("library_materials"));
    }
    XMLElement* material = nullptr;
    for (material = library_materials->FirstChildElement("material");
        material && !material->Attribute("id", matName.c_str());
        material = material->NextSiblingElement("material"));
    if (!material)
    {
        material = (XMLElement*)library_materials->InsertEndChild(xmlScene.NewElement("material"));
        material->SetAttribute("id", matName.c_str());

        XMLElement* effect = AddEffect(xmlScene, scene, matName + "-fx", "phong", diffTexName);
        XMLElement* instance_effect = (XMLElement*)material->InsertEndChild(xmlScene.NewElement("instance_effect"));
        instance_effect->SetAttribute("url", ("#" + matName + "-fx").c_str());
    }
    return material;
}

XMLElement * MeshPlugin::AddTexture(XMLDocument & xmlScene, XMLElement * scene, string diffTexName)
{
    XMLElement* library_images = scene->FirstChildElement("library_images");
    if (!library_images)
    {
        library_images = (XMLElement*)scene->InsertEndChild(xmlScene.NewElement("library_images"));
    }
    XMLElement* image = nullptr;
    for (image = library_images->FirstChildElement("image");
        image && !image->Attribute("id", diffTexName.c_str());
        image = image->NextSiblingElement("image"));
    if (!image)
    {
        image = (XMLElement*)library_images->InsertEndChild(xmlScene.NewElement("image"));
        image->SetAttribute("id", diffTexName.c_str());

        XMLElement* init_from = (XMLElement*)image->InsertEndChild(xmlScene.NewElement("init_from"));
        init_from->SetText(("file:///" + diffTexName + ".dds").c_str());
    }
    return image;
}

XMLElement * MeshPlugin::AddEffect(XMLDocument & xmlScene, XMLElement * scene, string effectName, string techniqueName, string diffTexName)
{
    XMLElement* library_effects = scene->FirstChildElement("library_effects");
    if (!library_effects)
    {
        library_effects = (XMLElement*)scene->InsertEndChild(xmlScene.NewElement("library_effects"));
    }
    XMLElement* effect = nullptr;
    for (effect = library_effects->FirstChildElement("effect");
        effect && !effect->Attribute("id", effectName.c_str());
        effect = effect->NextSiblingElement("effect"));

    if (!effect)
    {
        effect = (XMLElement*)library_effects->InsertEndChild(xmlScene.NewElement("effect"));
        effect->SetAttribute("id", effectName.c_str());

        auto profile_COMMON = (XMLElement*)effect->InsertEndChild(xmlScene.NewElement("profile_COMMON"));
        XMLElement* texture = AddTexture(xmlScene, scene, diffTexName);

        //	add surface
        auto newparam = (XMLElement*)profile_COMMON->InsertEndChild(xmlScene.NewElement("newparam"));
        newparam->SetAttribute("sid", (diffTexName + "-surface").c_str());
        auto surface = (XMLElement*)newparam->InsertEndChild(xmlScene.NewElement("surface"));
        surface->SetAttribute("type", "2D");
        auto init_from = (XMLElement*)surface->InsertEndChild(xmlScene.NewElement("init_from"));
        init_from->SetText(diffTexName.c_str());
        auto format = (XMLElement*)surface->InsertEndChild(xmlScene.NewElement("format"));
        format->SetText("A8R8G8B8");


        //	add sampler
        auto newparam1 = (XMLElement*)profile_COMMON->InsertEndChild(xmlScene.NewElement("newparam"));
        newparam1->SetAttribute("sid", (diffTexName + "-sampler").c_str());
        auto sampler2D = (XMLElement*)newparam1->InsertEndChild(xmlScene.NewElement("sampler2D"));
        auto source = (XMLElement*)sampler2D->InsertEndChild(xmlScene.NewElement("source"));
        source->SetText((diffTexName + "-surface").c_str());
        auto wrap_s = (XMLElement*)sampler2D->InsertEndChild(xmlScene.NewElement("wrap_s"));
        wrap_s->SetText("WRAP");
        auto wrap_t = (XMLElement*)sampler2D->InsertEndChild(xmlScene.NewElement("wrap_t"));
        wrap_t->SetText("WRAP");
        auto minfilter = (XMLElement*)sampler2D->InsertEndChild(xmlScene.NewElement("minfilter"));
        minfilter->SetText("NONE");
        auto magfilter = (XMLElement*)sampler2D->InsertEndChild(xmlScene.NewElement("magfilter"));
        magfilter->SetText("NONE");
        auto mipfilter = (XMLElement*)sampler2D->InsertEndChild(xmlScene.NewElement("mipfilter"));
        mipfilter->SetText("NONE");



        auto technique = (XMLElement*)profile_COMMON->InsertEndChild(xmlScene.NewElement("technique"));
        technique->SetAttribute("sid", "standard");
        auto phong = (XMLElement*)technique->InsertEndChild(xmlScene.NewElement("phong"));

        auto diffuse = (XMLElement*)phong->InsertEndChild(xmlScene.NewElement("diffuse"));
        auto texture1 = (XMLElement*)diffuse->InsertEndChild(xmlScene.NewElement("texture"));
        texture1->SetAttribute("texture", (diffTexName + "-sampler").c_str());
    }
    return effect;
}

BoneHeader * fileHeader::getBoneHeader()
{
    return (BoneHeader*)((char*)this + (FILE_BYTE * 8));
}

SceneHeader * fileHeader::getSceneHeader()
{
    return (SceneHeader*)((char*)this + (FILE_BYTE * 8) + 20 + (NR_OF_MESHES * 4) + PTR_MESH_START);
}

size_t fileHeader::getMeshHeaderOffset()
{
    return ((size_t)FILE_BYTE * 8) + 20 + ((size_t)NR_OF_MESHES * 4) + (size_t)PTR_MESH_START;
}

MeshHeader * SceneHeader::getMeshHeader(int meshIdx)
{
    return (MeshHeader*)(((char*)this + PTR_MESH_TBL00) + (meshIdx * sizeof(MeshHeader)));
}

ComponentDataBlob * SceneHeader::getComponentBlob(int meshIdx)
{
    return (ComponentDataBlob*)((char*)this + getMeshHeader(meshIdx)->vfOffset);
}

void * SceneHeader::getComponentData(int meshIdx)
{
    return (void*)((char*)this + getMeshHeader(meshIdx)->vertOffset);
}

FaceHeader * SceneHeader::getFaceHeader(int faceIdx)
{
    return (FaceHeader*)((char*)this + PTR_MAT_TABLE + (faceIdx * sizeof(FaceHeader)));
}

uint16_t * SceneHeader::getIndexBuffer(int faceIdx)
{
    return (uint16_t*)((char*)this + PTR_FACES_TABLE + getFaceHeader(faceIdx)->getIndexBufferOffset());
}

ComponentValue * ComponentDataBlob::getComponents(int componentIdx)
{
    return ((ComponentValue*)&this[1]) + componentIdx;
}

size_t FaceHeader::getIndexBufferOffset()
{
    return indexFrom * sizeof(uint16_t);
}

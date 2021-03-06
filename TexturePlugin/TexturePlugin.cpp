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
#include <stdint.h>
#include <string>
#include <string.h>
#include "DDSLoader.h"
#include "TexturePlugin.h"

using namespace std;

int createPluginInterface(PluginInterface **ppPluginInterface, const PluginCreateInfo &pluginCreateInfo) {
    if (!ppPluginInterface) {
        return 1;
    }
    *ppPluginInterface = new TexturePlugin(pluginCreateInfo.gameVersion);
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

TexturePlugin::TexturePlugin(uint32_t gameVer) : PluginInterface(gameVer) {
}

TexturePlugin::~TexturePlugin() {
}

int TexturePlugin::check(void *ptr, size_t sz, CDRM_TYPES &type) {
    if (((uint32_t *) ptr)[0] == 0x39444350) {
        type = CDRM_TYPE_DDS;
        return 1;
    }
    return 0;
}

int TexturePlugin::unpack(void *pDataIn, size_t sz, void **ppDataOut, size_t &szOut, CDRM_TYPES &type) {
    if (getGameVersion() == 3) {
        PCD9_Header *pcd9 = (PCD9_Header *) pDataIn;
        printf("bpp [%#x],u1 [%#x],u2 [%#x]\n", pcd9->bpp, pcd9->u1, pcd9->u2);
        DDS dds(pcd9->ddsFlags, pcd9->height, pcd9->width, 0, pcd9->mipmap, pcd9->format, pcd9->getDataPtr(), pcd9->size);
        szOut = dds.getDDSSize();
        *ppDataOut = new char[szOut];
        dds.writeToMemory((char *) *ppDataOut);
        type = CDRM_TYPE_DDS;
        return 0;
    } else if (getGameVersion() == 4) {
        uint32_t     format = 0;
        PCD11_Header *pcd11 = (PCD11_Header *) pDataIn;
        switch (pcd11->format) {
            case 0x47:
            case 0x48:
                format = MAKE_FOURCC('D', 'X', 'T', '1');
                break;
            case 0x4d:
            case 0x4e:
                format = MAKE_FOURCC('D', 'X', 'T', '3');
                break;
            case 0x50:
                format = MAKE_FOURCC('A', 'T', 'I', '1');
                break;
            case 0x53:
                format = MAKE_FOURCC('A', 'T', 'I', '2');
                break;
            case 0x57:
                // wrong
                format = MAKE_FOURCC('A', 'T', 'I', '2');
                break;
            case 0x5b:
                // wrong
                format = MAKE_FOURCC('D', 'X', 'T', '1');
                break;
            case 0x62:
                // wrong
                format = MAKE_FOURCC('D', 'X', 'T', '1');
                break;
            default:
                printf("pcd format %#x not implemented!\n", pcd11->format);
                exit(-1);
        }
        printf("bpp [%#x],u1 [%#x],u2 [%#x]\n", pcd11->bpp, pcd11->u1, pcd11->u2);
        DDS dds(pcd11->ddsFlags, pcd11->height, pcd11->width, 0, pcd11->mipmap, format, pcd11->getDataPtr(), pcd11->size);
        szOut = dds.getDDSSize();
        *ppDataOut = new char[szOut];
        dds.writeToMemory((char *) *ppDataOut);
        type = CDRM_TYPE_DDS;
        return 0;
    } else {
        printf("Game version not supported!");
        exit(-1);
        return -1;
    }
}

int TexturePlugin::pack(void *pDataIn, size_t sz, void **ppDataOut, size_t &szOut, CDRM_TYPES &type) {
    if (*(uint32_t *) pDataIn != _DDS_MAGIC) {
        return 1;
    }
    //	allocate PCD9 memory
    szOut = sz - sizeof(DDS_Header) - sizeof(uint32_t) + sizeof(PCD9_Header);
    *ppDataOut = new char[szOut];
    // decode
    DDS_Header  *pDDSHeader = (DDS_Header *) ((uint32_t *) pDataIn + 1);
    PCD9_Header *pcd9       = (PCD9_Header *) *ppDataOut;
    pcd9->magic    = PCD9_MAGIC;
    pcd9->format   = pDDSHeader->ddspf.dwFourCC;
    pcd9->size     = (uint32_t) sz - pDDSHeader->dwSize - sizeof(uint32_t);
    pcd9->u1       = 0;
    pcd9->width    = pDDSHeader->dwWidth;
    pcd9->height   = pDDSHeader->dwHeight;
    pcd9->bpp      = 1;
    pcd9->u2       = 0;
    pcd9->mipmap   = pDDSHeader->dwMipMapCount;
    pcd9->ddsFlags = pDDSHeader->dwFlags;
    //	copy image data
    memcpy(pcd9->getDataPtr(), (char *) (pDDSHeader + 1), pcd9->size);
    type = CDRM_TYPE_DDS;
    return 0;
}

int TexturePlugin::getType(void *, size_t) {
    return 0;
}

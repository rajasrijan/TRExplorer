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
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef __unix__
#include <dlfcn.h>
#else
#include <windows.h>
#define dlopen(x, y) LoadLibraryA(x)
#define dlsym(x, y) (void *)GetProcAddress((HMODULE)x, y)
#define dlclose(x)  CloseHandle(x);
#endif
#include "PluginInterface.h"
#include <stdio.h>

#define NAME_LENGTH 256

int main(int argv, char *argc[])
{
    if (argv != 4)
    {
        char *pExeName = strrchr(argc[0], '\\');
        printf("%s <plugin name> <input file> <output file>\n", pExeName ? pExeName + 1 : "PluginTest");
        return -1;
    }
    int errCode = 0;
    void *pfnCreate, *pfnDestroy;
    void *hPluginDll = NULL;
    PluginInterface *pPluginInterface = nullptr;
    int hInpFile = 0, hOpFile = 0;
    char pluginName[NAME_LENGTH] = {0};
    size_t fileSize = 0;
    //	read plugin name
    strcpy(pluginName, argc[1]);
    //	append ".dll"
    strcat(pluginName, ".so");
    //	try to load library.
    hPluginDll = dlopen(pluginName, RTLD_NOW);
    if (hPluginDll == NULL)
    {
        printf("Failed to load Plugin (%s)\n", pluginName);
        return 1;
    }
    //	try to get create and destroy fn().

    pfnCreate = dlsym(hPluginDll, "createPluginInterface");
    pfnDestroy = dlsym(hPluginDll, "destroyPluginInterface");
    if (pfnCreate == NULL || pfnDestroy == NULL)
    {
        dlclose(hPluginDll);
        printf("Failed to load interface api.\n");
        return 1;
    }
    //	create interface
    errCode = ((createPluginInterface)pfnCreate)(&pPluginInterface);
    if (errCode)
    {
        dlclose(hPluginDll);
        printf("Failed to create interface.\n");
        return 1;
    }
    hInpFile = open(argc[2], O_RDONLY);
    if (hInpFile == 0)
    {
        dlclose(hPluginDll);
        printf("Unable to open file.\n");
        return 1;
    }
    hOpFile = open(argc[3], O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (hOpFile == 0)
    {
        dlclose(hPluginDll);
        printf("Unable to open file.\n");
        return 1;
    }
    struct stat st = {};
    fstat(hInpFile, &st);
    fileSize = st.st_size;
    char *fileData = new char[fileSize];
    void *outputData = nullptr;
    size_t outputSize = 0;
    uint32_t sizeRead = 0;
    CDRM_TYPES type;
    if (!read(hInpFile, fileData, fileSize))
    {
        dlclose(hPluginDll);
        printf("Unable to read file.\n");
        return 1;
    }
    if (pPluginInterface->unpack(fileData, fileSize, &outputData, outputSize, type))
    {
        dlclose(hPluginDll);
        printf("Failed to unpack.\n");
        return 1;
    }
    uint32_t bytesWritten = 0;
    if (!write(hOpFile, outputData, outputSize))
    {
        dlclose(hPluginDll);
        printf("Unable to write file.\n");
        return 1;
    }
    close(hOpFile);
    close(hInpFile);
    return 0;
}

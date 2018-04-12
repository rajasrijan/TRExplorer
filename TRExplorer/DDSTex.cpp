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

#include "DDSTex.h"
#include <IL/il.h>
#include <IL/ilu.h>

int LoadFromDDSMemory(void* pDataIn, size_t size, size_t width, size_t height, void* pDataOut)
{
	int ret = 0;
	ilInit();

	if (!ilLoadL(IL_DDS, pDataIn, size))
	{
		printf("Failed to load image from memory\n");
		ret = 1;
		goto error_exit;
	}

	if (!iluScale(width, height, 1))
	{
		printf("Failed to scale image\n");
		ret = 1;
		goto error_exit;
	}
	//ret = ilSaveL(IL_RGBA, pDataOut, width * height * 4);
	if (!ilCopyPixels(0, 0, 0, width, height, 1, IL_RGB, IL_UNSIGNED_BYTE, pDataOut))
	{
		printf("Failed to save image to memory\n");
		ret = 1;
		goto error_exit;
	}
	error_exit:

	ilShutDown();
	return ret;
}

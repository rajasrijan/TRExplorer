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
#include <stdint.h>

//	ASCII value of "PCD9"
#define PCD9_MAGIC 0x39444350

#pragma pack(push,1)
struct PCD9_Header
{
	union
	{
		uint32_t magic;
		char magicStr[4];
	};
	union
	{
		uint32_t format;
		char formatName[4];
	};
	uint32_t size;
	uint32_t u1;
	uint16_t width, height;
	uint8_t bpp;
	uint16_t u2;
	uint8_t mipmap;
	uint32_t ddsFlags;
	char* getDataPtr()
	{
		return (char*)(&this[1]);
	}
};
struct PCD11_Header
{
	union
	{
		uint32_t magic;
		char magicStr[4];
	};
	union
	{
		uint32_t format;
		char formatName[4];
	};
	uint32_t size;
	uint32_t u1;
	uint16_t width, height;
	uint8_t bpp;
	uint16_t u2;
	uint8_t mipmap;
	uint32_t ddsFlags;
	char* getDataPtr()
	{
		return (char*)(&this[1]);
	}
};
#pragma pack(pop)

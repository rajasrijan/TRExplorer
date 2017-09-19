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

#include <DirectXMath.h>

#define MAKEVERSION(x,y)	(((x&0xFFFF)<<16)|(y&0xFFFF))
#define VERSION_MAJOR		1
#define VERSION_MINOR		0
#define VERSION				MAKEVERSION(VERSION_MAJOR,VERSION_MINOR)

#ifdef PLUGIN
#define API extern "C" __declspec(dllexport) 
#else
#define API
#endif // PLUGIN

enum CDRM_TYPES
{
	CDRM_TYPE_UNKNOWN = 0,
	CDRM_TYPE_DDS = 1,
	CDRM_TYPE_MESH = 2,
};

struct SimpleVertex
{
	DirectX::XMFLOAT3	mPos;
	DirectX::XMFLOAT3	mNor;
	DirectX::XMFLOAT3	mTex;
	DirectX::XMMATRIX	mTran;
	uint32_t			mBoneIndex;
	DirectX::XMMATRIX	mBoneWeight;
};

class PluginInterface
{
public:
	PluginInterface()
	{
	}

	~PluginInterface()
	{
	}
	virtual int check(void*, size_t, CDRM_TYPES& type) = 0;
	virtual int unpack(void*, size_t, void**, size_t&, CDRM_TYPES&) = 0;
	virtual int pack(void*, size_t, void**, size_t&, CDRM_TYPES&) = 0;
	virtual int getType(void*, size_t) = 0;
	virtual uint32_t getPluginInterfaceVersion() { return VERSION; }
private:

};

#ifndef PLUGIN
typedef int(*createPluginInterface)(PluginInterface** ppPluginInterface);
typedef int(*destroyPluginInterface)(PluginInterface* pPluginInterface);
#endif // !PLUGIN

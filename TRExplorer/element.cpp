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
#include "element.h"

element_t::element_t(void* ele, int ver) :p_element(ele), version(ver)
{
	memset(sName, 0, _countof(sName));
}

element_t::~element_t()
{
}

const char * element_t::getName()
{
	if (sName[0] == 0)
	{
		uint32_t hash = (version == 4) ? ((element_v2*)p_element)->getNameHash() : ((element_v1*)p_element)->getNameHash();
		auto NameIt = fileListHashMap.find(hash);
		if (NameIt == fileListHashMap.end())
		{
			sprintf_s(sName, "%#x", hash);
		}
		else
		{
			strcpy_s(sName, NameIt->second.c_str());
		}
	}
	return sName;
}

void * element_t::getElement()
{
	return p_element;
}

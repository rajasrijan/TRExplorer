#pragma once
#include <string>
#include <stdint.h>

using namespace std;

#define DDPF_ALPHAPIXELS	0x1
#define DDPF_ALPHA			0x2
#define DDPF_FOURCC			0x4
#define DDPF_RGB			0x40
#define DDPF_YUV			0x200
#define DDPF_LUMINANCE		0x20000

struct DDS_PIXELFORMAT
{
	uint32_t dwSize;
	uint32_t dwFlags;
	uint32_t dwFourCC;
	uint32_t dwRGBBitCount;
	uint32_t dwRBitMask;
	uint32_t dwGBitMask;
	uint32_t dwBBitMask;
	uint32_t dwABitMask;
};

struct DDS_Header
{
	uint32_t dwSize;
	uint32_t dwFlags;
	uint32_t dwHeight;
	uint32_t dwWidth;
	uint32_t dwPitchOrLinearSize;
	uint32_t dwDepth;
	uint32_t dwMipMapCount;
	uint32_t dwReserved1[11];
	DDS_PIXELFORMAT ddspf;
	uint32_t dwCaps;
	uint32_t dwCaps2;
	uint32_t dwCaps3;
	uint32_t dwCaps4;
	uint32_t dwReserved2;
};

class DDS
{
private:
	DDS_Header header;
	char* data;
	uint32_t dataSize;
public:
	DDS();
	DDS(uint32_t flags , uint32_t height , uint32_t width , uint32_t depth , uint32_t mip ,  uint32_t compresion ,char* _data , uint32_t _size);
	bool deserialization(string fileName);
	void serialization(string fileName);
	~DDS(void);
	uint32_t getDataSize();
	char * getData();
};


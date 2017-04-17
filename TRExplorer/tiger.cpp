#include "alldef.h"
map<uint32_t, void*> tigerPtrMap;

int unknown_header2::setSize(size_t newSize)
{
	CDRM_BlockFooter *footer = getCDRMFooter();
	CDRM_Header* nextCdrm = reinterpret_cast<CDRM_Header*>((char*)footer + footer->relative_offset);
	size = (uint32_t)newSize;
	CDRM_BlockFooter *newfooter = getCDRMFooter();
	((uint32_t*)newfooter->magic)[0] = 'TXEN';
	newfooter->relative_offset = (uint32_t)((char*)nextCdrm - (char*)newfooter);
	return 0;
}

CDRM_BlockFooter * unknown_header2::getCDRMFooter()
{
	if (tigerPtrMap.find(offset & TIGER_FILEID_MASK) != tigerPtrMap.end())
	{
		return (CDRM_BlockFooter*)((char*)(tigerPtrMap[offset & TIGER_FILEID_MASK]) + (offset&(~TIGER_FILEID_MASK)) + (((size + 0xF) / 0x10) * 0x10));
	}
	else
	{
		printf("Error occured trying to read offset [%#X]", offset);
		exit(1);
		return NULL;
	}
}

uint32_t * StringTable_Header::getStringIndicesPtr()
{
	return (uint32_t*)((char*)(&this[1]) + (unknownCount * sizeof(uint32_t)));
}

uint32_t StringTable_Header::getStringIndicesCount()
{
	return tableRange;
}

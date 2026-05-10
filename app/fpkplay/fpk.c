#include <piece.h>
#include "fpk.h"

static FILEACC g_fpk;
static int g_sct = -1;
static uint8_t cache[4096]; // セクタキャッシュ

int fpkOpenArchive(char* filename, fpack_header_t* header)
{
	if(pceFileOpen(&g_fpk, filename, FOMD_RD) != 0)
		return -1;

    fpkFileReadPos((unsigned char*)header, 0, sizeof(fpack_header_t));
	if(memcmp(header->signature, "KAPF", 4) != 0)
	{
		pceFileClose(&g_fpk);
		return -1;
	}
	return 0;
}

void fpkCloseArchive()
{
    pceFileClose(&g_fpk);
}

int fpkFileReadPos(unsigned char* buf, unsigned int pos, int size)
{
	int rem = size;
	int sct;
	int len;

	sct = pos / 4096;
	pos %= 4096;
	while(rem > 0) {
		if(g_sct != sct) {
			pceFileReadSct(&g_fpk, cache, sct, 4096);
			g_sct = sct;
		}
		len = 4096 - pos;
		if(len > rem) {
			len = rem;
		}
		memcpy(buf, cache + pos, len);
		buf += len;
		rem -= len;
		sct++;
		pos = 0;
	}

	return size;
}

int fpkGetFileInfoN(int index, fpack_file_entry_t* entry)
{
    fpkFileReadPos((unsigned char*)entry, sizeof(fpack_header_t) + index * sizeof(fpack_file_entry_t), sizeof(fpack_file_entry_t));

	return 0;
}

int fpkExtractToBuffer(fpack_file_entry_t* entry, void* buffer)
{
	fpkFileReadPos(buffer, entry->offset, entry->size);

	return 0;
}

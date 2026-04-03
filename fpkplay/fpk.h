#if !defined(FPK_H)
#define FPK_H

#include "pcestdint.h"
#include "fpack.h"
#include <string.h>

int fpkOpenArchive(char* filename, fpack_header_t* header);
int fpkFileReadPos(unsigned char* buf, unsigned int pos, int size);
int fpkGetFileInfoN(int index, fpack_file_entry_t* entry);
int fpkExtractToBuffer(fpack_file_entry_t* entry, void* buffer);
void fpkCloseArchive();


#endif // !defined(FPK_H)

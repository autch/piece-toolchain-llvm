#ifndef fpack_h
#define fpack_h

#include "pcestdint.h"

typedef struct {
    char signature[4];      // 'FPAK'
    uint32_t files_count;   // Number of files in the archive
} /*__attribute__((packed))*/ fpack_header_t;

typedef struct {
    char filename[16];      // Filename (null-terminated, max 15 chars + null terminator)
    uint32_t offset;        // Offset in the archive where the file data starts
    uint32_t size;          // Size of the file data, if compressed, this is the compressed size
    // decompressed size can be derived from the first 4 bytes of the file data
}  /*__attribute__((packed))*/ fpack_file_entry_t;

#endif // fpack.h

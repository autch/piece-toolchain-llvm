#ifndef lzss_h
#define lzss_h

#include "pcestdint.h"

#define N 4096
#define F 18

uint32_t decodeLZSS(const uint8_t* pSrc, size_t srcSize, uint8_t* pDest, size_t destSize);
void encodeLZSS(const uint8_t* pSrc, uint32_t srcSize, uint8_t* pDest, uint32_t* pEncodedSize);

#endif // lzss_h

#include "pcestdint.h"
#include <string.h>

#include "lzss.h"

static uint8_t pTemp[N];

uint32_t decodeLZSS(const uint8_t* pSrc, size_t srcSize, uint8_t* pDest, size_t destSize)
{
    int r = N - F;
    uint8_t flags = 0;
    const uint8_t* psrcOrg = pSrc;
    int flagCount = 0;
    uint32_t decodedCount = 0;
    uint32_t decodedSize = pSrc[0] | (pSrc[1] << 8) | (pSrc[2] << 16) | (pSrc[3] << 24);
    uint32_t max_destsize = destSize < decodedSize ? destSize : decodedSize;

    pSrc += 4;

    memset(pTemp, 0, sizeof(pTemp));
    while (decodedCount < max_destsize && (pSrc - psrcOrg) < srcSize) {
        if (flagCount == 0) {
            flags = *pSrc++;
            flagCount = 8;
        }

        if (flags & 0x80) {
            // Literal byte
            pTemp[r++] = pDest[decodedCount++] = *pSrc++;
            r &= (N - 1);
        } else {
            // Reference
            uint8_t b1 = *pSrc++;
            uint8_t b2 = *pSrc++;
            int i;

            unsigned offset = b1 | ((b2 & 0xF0) << 4);
            unsigned length = (b2 & 0x0F) + 3;

            for (i = 0; i < length && decodedCount < max_destsize; i++) {
                pTemp[r++] = pDest[decodedCount++] = pTemp[(offset + i) & (N - 1)];
                r &= (N - 1);
            }
        }

        flags <<= 1;
        flagCount--;
    }
    // should match decodedSize
    return decodedCount;
}

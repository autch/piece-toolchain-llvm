/*
 * using instances via global variables
 */
#include <piece.h>

extern "C"
{
    void *memset(void *dst, int c, size_t n);
    void* memcpy(void *dst, const void *src, size_t n);
}

struct Counter {
    unsigned value;

    Counter() : value(0xdeadbeef) {} // Non-trivial constructor to test C++ static initialization.
};

Counter g_counter;

unsigned char vbuff[128 * 88];

extern "C"
{
    void pceAppInit(void)
    {
        memset(vbuff, 0, sizeof(vbuff));
        pceLCDDispStop();
        pceLCDSetBuffer(vbuff);
        pceLCDDispStart();
        pceFontSetPos(0, 0);
        pceFontPrintf("Counter: %08x", g_counter.value);

        pceLCDTrans();
    }

    void pceAppProc(int cnt)
    {
    }

    void pceAppExit(void)
    {
    }
}

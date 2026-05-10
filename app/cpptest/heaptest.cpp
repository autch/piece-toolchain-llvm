#include <piece.h>

extern "C"
{
    void *memset(void *dst, int c, size_t n);
    void* memcpy(void *dst, const void *src, size_t n);

    void pceAppInit();
    void pceAppProc(int cnt);
    void pceAppExit();

    extern unsigned char __END_DEFAULT_BSS[];

    void _exit(int status)
    {
        (void)status;
        for (;;) {}
    }
}

__attribute__((naked)) 
extern "C" void* get_SP()
{
    asm("ld.w %r10, %sp\nret\n");
}

unsigned char vbuff[128 * 88];

char* buf = nullptr;
char* buf2 = nullptr;

void pceAppInit(void)
{
    memset(vbuff, 0, sizeof(vbuff));
    pceLCDDispStop();
    pceLCDSetBuffer(vbuff);
    pceLCDDispStart();

    buf = new char[20];
    buf2 = (char*)pceHeapAlloc(20);

    pceFontSetPos(0, 0);
    pceFontPrintf("new char[20]: %p", buf);
    pceFontSetPos(0, 10);
    pceFontPrintf("pceHeapAlloc(20): %p", buf2);

    pceFontSetPos(0, 20);
    pceFontPrintf("BSS_END: %p", __END_DEFAULT_BSS);

    pceFontSetPos(0, 30);
    pceFontPrintf("(current) SP: %p", get_SP());

    pceLCDTrans();
}

void pceAppProc(int cnt)
{
}

void pceAppExit(void)
{
    delete[] buf;
    pceHeapFree(buf2);
}

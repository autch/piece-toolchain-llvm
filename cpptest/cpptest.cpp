#include <piece.h>
#include <string.h>

extern "C"
{
    extern unsigned char _def_vbuff[128 * 88];

    void _exit(int status) {
        (void)status;
        for (;;) {}
    }
}

class AppBase
{
public:
    virtual ~AppBase() = default;

    virtual void init() = 0;
    virtual void run(int cnt) = 0;
    virtual void exit() = 0;
    virtual int notify(int type, int param) = 0;
};

// LCD buffer with guard padding — kernel LCD routines write outside buffer bounds.
// The kernel writes up to ~20 bytes BEFORE and ~4 bytes AFTER the buffer passed
// to pceLCDSetBuffer().  64-byte guards absorb this overflow.
// Declared FIRST among static BSS variables so that underflow writes go toward
// lower addresses (.text) rather than toward app/heap pointers.
struct alignas(4) LCDBuffer
{
    unsigned char pre_guard[64];
    unsigned char pixels[128 * 88];
    unsigned char post_guard[8];
};
static LCDBuffer lcd_buf;
// App code uses &lcd_buf.pixels[0] via this alias
static unsigned char *const vbuff = lcd_buf.pixels;

class App : public AppBase
{
public:
    App()
    {
        for (int i = 0; i < 128 * 88; ++i)
            vbuff[i] = 3;
    }
    virtual ~App() = default;

    virtual void init() override
    {
        memset(vbuff, 0, 128 * 88);
        pceLCDDispStop();
        pceLCDSetBuffer(vbuff);
        pceLCDDispStart();
        pceFontSetPos(0, 0);
        pceFontSetTxColor(3);
        pceFontPrintf("Hello from C++ world!");
        pceLCDTrans();
    }
    virtual void run(int cnt) override
    {
    }
    virtual void exit() override {}
    virtual int notify(int type, int param) override
    {
        switch (type)
        {
        case APPNF_SMSTART:
            return APPNR_ACCEPT;
        case APPNF_SMREQVBUF:
            pceLCDSetBuffer(_def_vbuff);
            return APPNR_ACCEPT;
        }
        return APPNR_IGNORE;
    }

private:
};

AppBase *app = nullptr;

// Diagnostic — all BSS
static volatile unsigned long diag_proc_count = 0;

extern "C"
{

    void pceAppInit(void)
    {
        app = new App();
        app->init();

        // Diagnostic overlay
        pceFontSetPos(0, 72);
        pceFontPrintf("I:%x", (unsigned int)app);
        pceLCDTrans();
    }

    void pceAppProc(int cnt)
    {
        diag_proc_count++;
        // Diagnostic display
        pceFontSetPos(0, 0);
        pceFontPrintf("P:%x #%d", (unsigned int)app, (int)diag_proc_count);
        pceLCDTrans();

        return;

        // Virtual dispatch via app directly
        app->run(cnt);

        if (pcePadGet() & TRG_D)
            pceAppReqExit(0);
    }

    void pceAppExit(void)
    {
        if (app)
        {
            app->exit();
            delete app;
            app = nullptr;
        }
    }

    int pceAppNotify(int type, int param)
    {
        return app->notify(type, param);
    }
}

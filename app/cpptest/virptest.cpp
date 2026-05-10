#include <piece.h>

extern "C"
{
    extern unsigned char _def_vbuff[128 * 88];
    void *memset(void *dst, int c, size_t n);

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

class App : public AppBase
{
public:
    App() {
        memset(vbuff, 0, sizeof(vbuff));    // non-trivial constructor, so this object is in BSS, not in .data
    }
    virtual ~App() = default;

    virtual void init() override
    {
        pceLCDDispStop();
        pceLCDSetBuffer(vbuff);
        pceLCDDispStart();
        pceFontSetPos(0, 0);
        pceFontPrintf("Hello from C++ world!");
        pceLCDTrans();
    }
    virtual void run(int cnt) override
    {
        pceFontSetPos(0, 10);
        pceFontPrintf("Count: %d", cnt);
        pceLCDTrans();
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
    unsigned char vbuff[128 * 88];
};

App g_app;
App* app = nullptr;  // pointer to BSS object, NOT heap

extern "C"
{

    void pceAppInit(void)
    {
        app = &g_app;

        app->init();
    }

    void pceAppProc(int cnt)
    {
        app->run(cnt);
    }

    void pceAppExit(void)
    {
        app->exit();
    }
}

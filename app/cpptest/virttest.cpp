/*
 * using derived class with virtual functions and virtual dispatch
 * instance itself is allocated as a global variable, but virtual dispatch is tested in various contexts (init/proc/exit/notify)
 */
#include <piece.h>

extern "C"
{
    extern unsigned char _def_vbuff[128 * 88];
    void *memset(void *dst, int c, size_t n);
}

unsigned char vbuff[128 * 88];

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
    App() = default;
    virtual ~App() = default;

    virtual void init() override
    {
        memset(vbuff, 0, sizeof(vbuff));
        pceLCDDispStop();
        pceLCDSetBuffer(vbuff);
        pceLCDDispStart();
        pceFontSetPos(0, 0);
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
};

App app;

extern "C"
{

    void pceAppInit(void)
    {
        app.init();
    }

    void pceAppProc(int cnt)
    {
        app.run(cnt);
    }

    void pceAppExit(void)
    {
        app.exit();
    }

    int pceAppNotify(int type, int param)
    {
        return app.notify(type, param);
    }
}

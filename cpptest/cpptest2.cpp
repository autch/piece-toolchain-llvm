#include <piece.h>
#include <string.h>

extern "C"
{
    extern unsigned char _def_vbuff[128 * 88];

    void pceAppInit(void);
    void pceAppProc(int cnt);
    void pceAppExit(void);
    int pceAppNotify(int type, int param);
}

class AppBase
{
public:
    virtual ~AppBase() = default;

    virtual void init() = 0;
    virtual void run(int cnt) = 0;
    virtual void exit() = 0;
    virtual int notify(int type, int param)
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

class App : public AppBase
{
public:
    App(): AppBase() {
        for(int i = 0; i < 128 * 88; ++i)
            vbuff[i] = 0;
    }
    virtual ~App() = default;

    virtual void init() override
    {
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
private:
    unsigned char vbuff[128 * 88];
};

// Diagnostic — all BSS
static volatile unsigned long diag_proc_count = 0, diag_notify_count = 0;

AppBase* app = nullptr;


void pceAppInit(void)
{
    app = new App();
    app->init();

    // Diagnostic overlay
    pceFontSetPos(0, 78);
    pceFontPrintf("I:%x", (unsigned int)app);
    pceLCDTrans();
}

void pceAppProc(int cnt)
{
    diag_proc_count++;

    // Virtual dispatch via app directly
    app->run(cnt);

    // Diagnostic display
    pceFontSetPos(0, 10);
    pceFontPrintf("P:%x #%d", (unsigned int)app, (int)diag_proc_count);
    pceLCDTrans();

    if(pcePadGet() & TRG_D) pceAppReqExit(0);
}

void pceAppExit(void)
{
    if (app) {
        app->exit();
        delete app;
        app = nullptr;
    }
}

int pceAppNotify(int type, int param)
{
    diag_notify_count++;
    pceFontSetPos(0, 20);
    pceFontPrintf("N:%x, %d", (unsigned int)app, (int)diag_notify_count);
    
    // この行を有効にするとクラッシュする
    return app->notify(type, param);
    return APPNR_ACCEPT;
}

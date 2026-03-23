#include <piece.h>

extern unsigned char __START_DEFAULT_BSS[];
extern unsigned char __END_DEFAULT_BSS[];
extern unsigned char _stacklen[];

unsigned char _def_vbuff[128*88];

/* required user callbacks */
extern void pceAppInit( void );
extern void pceAppProc( int cnt );
extern void pceAppExit( void );

/*
 * Weak default implementations of the user callbacks.
 * Applications override these by providing their own definitions.
 */
__attribute__((weak))
int pceAppNotify(int type, int param)
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

static void pceAppInit00(void);
static void pceAppProc00(int cnt);
static void pceAppExit00(void);
static int pceAppNotify00(int type, int param);

__attribute__((used))
static const pceAPPHEAD pceAppHead = {
    APPSIG,
    APPSYSVER,
    0,
    pceAppInit00,
    pceAppProc00,
    pceAppExit00,
    pceAppNotify00,
    (unsigned long)_stacklen,
    __END_DEFAULT_BSS,
};

static void pceAppInit00(void)
{
    for(unsigned char* p = __START_DEFAULT_BSS; p != __END_DEFAULT_BSS; p++)
      *p = 0;
    pceAppInit();
}
static void pceAppProc00(int cnt) { pceAppProc(cnt); }
static void pceAppExit00(void) { pceAppExit(); }

/*
 * Framework-level notification handler.
 * APPNF_SMREQVBUF is handled here: we register __START_DEFAULT_BSS as the
 * LCD framebuffer so the user never needs to call pceLCDSetBuffer themselves.
 * All other notifications are forwarded to the user's pceAppNotify().
 */
static int pceAppNotify00(int type, int param)
{
    return pceAppNotify(type, param);
}


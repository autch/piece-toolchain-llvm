#include <piece.h>
#include <string.h>

extern unsigned char __START_DEFAULT_BSS[];
extern unsigned char __END_DEFAULT_BSS[];
extern unsigned char __SIZEOF_DEFAULT_BSS[];

extern unsigned char _stacklen[];

extern void pceAppInit( void );
extern void pceAppProc( int cnt );
extern void pceAppExit( void );
extern int pceAppNotify( int type, int param );

extern int __version_check( int opef );

static void pceAppInit00( void );
static void pceAppProc00( int cnt );
static void pceAppExit00( void );
static int pceAppNotify00( int type, int param );

/* __attribute__((used)): The kernel accesses pceAppHead by address (0x100000)
 * without going through any C symbol reference.  Without this attribute,
 * Clang/LLVM eliminates the symbol regardless of optimisation level because
 * it sees no in-TU reference to a static-linkage object. */
__attribute__((used)) static const pceAPPHEAD pceAppHead = {
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

static void pceAppInit00( void )
{
	memset( __START_DEFAULT_BSS, 0, __SIZEOF_DEFAULT_BSS );
	if ( __version_check(0) ) return;
	pceAppInit();
}

static void pceAppProc00( int cnt )
{
	if ( __version_check(cnt) ) return;
	pceAppProc( cnt );
}

static void pceAppExit00( void )
{
	if ( __version_check(0) ) return;
	pceAppExit();
}

static int pceAppNotify00( int type, int param )
{
	if ( __version_check(0) ) return APPNR_IGNORE;
	return pceAppNotify( type, param );
}

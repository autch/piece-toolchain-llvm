#include <piece.h>

extern unsigned char __START_DEFAULT_BSS[];
extern unsigned char __END_DEFAULT_BSS[];

extern unsigned char _stacklen[];

// Flag to indicate whether the CRT has been initialized.  This is used to prevent
// calling app code before initialization is complete, which could happen if the
// kernel sends a notification during initialization.
static int crt_initialized = 0;

/* .init_array boundaries (defined in linker script piece.ld) */
extern void (*__init_array_start[])(void);
extern void (*__init_array_end[])(void);

extern void* memset( void *s, int c, unsigned long n );

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
	/* Clear BSS.  Both __START_DEFAULT_BSS and __END_DEFAULT_BSS 
	 * are 4-byte aligned (linker script ALIGN(4)).
	 */
	unsigned long* bss = (unsigned long*)__START_DEFAULT_BSS;
	unsigned long* end = (unsigned long*)__END_DEFAULT_BSS;
	while (bss < end) {
		*bss++ = 0;
	}

	if ( __version_check(0) ) return;

	/* Call C++ static constructors / __attribute__((constructor)) */
	{
		void (**fn)(void) = __init_array_start;
		while (fn < __init_array_end) {
			(*fn)();
			fn++;
		}
	}

	pceAppInit();

	crt_initialized = 1;
}

static void pceAppProc00( int cnt )
{
	if ( __version_check(cnt) ) return;
	if( !crt_initialized ) return;
	pceAppProc( cnt );
}

static void pceAppExit00( void )
{
	if ( __version_check(0) ) return;
	if( !crt_initialized ) return;
	pceAppExit();
}

static int pceAppNotify00( int type, int param )
{
	if ( __version_check(0) ) return APPNR_IGNORE;
	if( !crt_initialized ) return APPNR_IGNORE;
	return pceAppNotify( type, param );
}

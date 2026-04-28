#include <piece.h>
#include <stdint.h>

extern uint8_t __START_DEFAULT_BSS[];
extern uint8_t __END_DEFAULT_BSS[];

/* Address reported to the kernel via pceAPPHEAD.bss_end.  Kernel uses
 * this as its SRAM heap base (see InitHeapAndSP / ResetHeap in
 * sdk/sysdev/pcekn/runapp.c), so it must sit at or above
 * __END_DEFAULT_BSS.  piece.ld provides _pceheapstart as a PROVIDE
 * default of __END_DEFAULT_BSS, overridable via -Wl,--defsym. */
extern uint8_t _pceheapstart[];

/* Internal-RAM placed sections (defined in piece.ld).
 * All symbols are 4-byte aligned.  When a section is unused, start==end and
 * the copy/clear loops below run zero iterations. */
extern uint8_t __fastrun_start[];
extern uint8_t __fastrun_end[];
extern uint8_t __fastrun_load[];
extern uint8_t __fastdata_start[];
extern uint8_t __fastdata_end[];
extern uint8_t __fastdata_load[];
extern uint8_t __fastbss_start[];
extern uint8_t __fastbss_end[];

extern uint8_t _stacklen[];

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
	_pceheapstart,	/* = __END_DEFAULT_BSS by default; overridable */
};

static void __memcpy(void* dst, const void* src, const void* dst_end)
{
	uint32_t* ps = (uint32_t*)src;
	uint32_t* p = (uint32_t*)dst;
	const uint32_t* pe = (const uint32_t*)dst_end;
	while (p < pe) {
		*p++ = *ps++;
	}
}

static void __memset(void* dst, int c, const void* dst_end)
{
	uint32_t* p = (uint32_t*)dst;
	const uint32_t* pe = (const uint32_t*)dst_end;
	while (p < pe) {
		*p++ = (uint32_t)c;
	}
}

static void pceAppInit00( void )
{
	/* Clear BSS.  Both __START_DEFAULT_BSS and __END_DEFAULT_BSS 
	 * are 4-byte aligned (linker script ALIGN(4)).
	 */
	__memset(__START_DEFAULT_BSS, 0, __END_DEFAULT_BSS);

	/* Copy .fastrun (hot code) from SRAM LMA to IRAM VMA. */
	__memcpy(__fastrun_start, __fastrun_load, __fastrun_end);

	/* Copy .fastdata (hot initialised data) from SRAM LMA to IRAM VMA. */
	__memcpy(__fastdata_start, __fastdata_load, __fastdata_end);

	/* Clear .fastbss (hot zero-initialised data) in IRAM. */
	__memset(__fastbss_start, 0, __fastbss_end);

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

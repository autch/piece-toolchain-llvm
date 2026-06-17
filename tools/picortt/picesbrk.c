/*
 * picesbrk.c - sbrk() for picolibc malloc on S1C33 / P/ECE.
 *
 * picolibc's own picosbrk.c bounds the heap with __heap_start/__heap_end
 * and refuses to grow past __heap_end.  P/ECE has no link-time SRAM top
 * (system_info.sram_end is only known at runtime), and piece.ld defines
 * neither symbol, so we provide our own sbrk that mirrors the old newlib
 * _sbrk: the libc heap starts just above the kernel pceHeap zone
 * (_pceheapstart + _pceheapsize) and grows upward without an upper bound.
 *
 * This strong definition is linked ahead of -lc, so picolibc's picosbrk.o
 * (which would otherwise pull in __heap_start/__heap_end) is never
 * extracted from libc.a.
 *
 * Apps that never call malloc let --gc-sections drop this object entirely,
 * so the reservation costs nothing unless dynamic allocation is used.
 */

#include <stddef.h>
#include <errno.h>

/* Absolute symbols provided by piece.ld: the symbol's *address* is the
 * value.  _pceheapstart defaults to __END_DEFAULT_BSS; the kernel pceHeap
 * zone occupies the first _pceheapsize bytes, and the libc heap follows. */
extern char _pceheapstart[];
extern char _pceheapsize[];

void *
sbrk(ptrdiff_t incr)
{
    static char *brk = NULL;
    char *prev;

    if (brk == NULL)
        brk = _pceheapstart + (size_t) _pceheapsize;

    prev = brk;
    brk += incr;               /* unbounded, matches the old newlib _sbrk */
    return prev;
}

/*
 * rand / srand shim for newlib on P/ECE.
 *
 * Newlib's rand() routes through the reentrancy struct
 * (_REENT_CHECK_RAND48), which lazily mallocs a struct _rand48 on first
 * use and asserts on failure.  That one dependency drags malloc, free,
 * vfprintf, fvwrite, and the entire stdio init machinery into every
 * binary that calls rand() -- roughly 7 KB on P/ECE.
 *
 * P/ECE is single-threaded and has no need for newlib's reentrancy
 * machinery, so this shim provides a tiny LCG that shadows newlib's
 * rand/srand at link time.  RAND_MAX matches newlib's stdlib.h
 * (0x7fffffff on 32-bit int targets).
 *
 * S1C33209 has a hardware multiplier, so the 32-bit LCG step compiles
 * down to a single mlt.w instruction with no library dependency.
 */

static unsigned long _rand_state = 1;

void srand(unsigned int seed)
{
    _rand_state = seed;
}

int rand(void)
{
    _rand_state = _rand_state * 1103515245UL + 12345UL;
    return (int)((_rand_state >> 1) & 0x7fffffff);
}

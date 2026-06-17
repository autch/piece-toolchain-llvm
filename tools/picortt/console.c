/*
 * console.c - picolibc stdio retargeting for S1C33 / P/ECE.
 *
 * P/ECE has no character console.  Text destined for the screen goes
 * through pceFontPrintf, not the C stdio stream, so stdout/stderr output
 * is discarded here (a no-op putc), mirroring the behaviour of the old
 * newlib _write stub.  stdin reports immediate EOF so fgetc/scanf return
 * cleanly instead of blocking.
 *
 * picolibc's tinystdio expects the application (or its retarget layer) to
 * define stdin/stdout/stderr when neither the semihost nor posix-console
 * backends are built in.  We define a single static FILE and alias all
 * three streams to it, exactly as picolibc's own semihost/common/iob.c
 * does.  Our strong definitions override picolibc's weak stdin/stdout.
 */

#include <stdio.h>

static int
piece_putc(char c, FILE *file)
{
    (void) file;
    /* Discard the byte; report it as written so callers see success. */
    return (unsigned char) c;
}

static int
piece_getc(FILE *file)
{
    (void) file;
    /* No console input on P/ECE. */
    return _FDEV_EOF;
}

static FILE __stdio =
    FDEV_SETUP_STREAM(piece_putc, piece_getc, NULL, _FDEV_SETUP_RW);

#ifdef __strong_reference
#define STDIO_ALIAS(x) __strong_reference(stdin, x);
#else
#define STDIO_ALIAS(x) FILE *const x = &__stdio;
#endif

FILE *const stdin = &__stdio;
STDIO_ALIAS(stdout);
STDIO_ALIAS(stderr);

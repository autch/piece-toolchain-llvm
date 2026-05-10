/*
 * __assert / __assert_func shim for newlib on P/ECE.
 *
 * Newlib's __assert_func() calls fprintf(stderr, ...).  P/ECE has no
 * stderr -- there is no host process to receive the message -- so the
 * fprintf call is dead weight, and it drags vfprintf, fvwrite, malloc,
 * and the stdio init machinery into the binary.
 *
 * This shim replaces both symbols with a request-exit-then-spin loop,
 * which is the closest reasonable behavior for a freestanding app.
 */

extern void pceAppReqExit(int code);

__attribute__((noreturn))
void __assert_func(const char *file, int line,
                   const char *func, const char *expr)
{
    (void)file; (void)line; (void)func; (void)expr;
    pceAppReqExit(0);
    for (;;) { }
}

__attribute__((noreturn))
void __assert(const char *file, int line, const char *expr)
{
    __assert_func(file, line, (const char *)0, expr);
}

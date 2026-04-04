/*
 * halt.c - bare-metal abort() / _exit() stubs for S1C33 / P/ECE
 *
 * These are compiled WITHOUT -flto so they are native ELF objects.  LLVM's
 * LTO optimizer treats abort() as a well-known external library function: it
 * emits calls to "abort" in the LTO output rather than inlining any bitcode
 * definition.  Providing a native abort() here ensures the linker can satisfy
 * that reference without falling back to liblib.a's abort.o, which would
 * introduce an otherwise-unsatisfied reference to _exit.
 *
 * abort() is a strong symbol so it shadows liblib.a's abort.o entirely.
 * _exit() is weak so application code may override it if needed.
 */

__attribute__((noreturn)) void abort(void) {
    for (;;) {}
}

__attribute__((noreturn, weak)) void _exit(int status) {
    (void)status;
    for (;;) {}
}

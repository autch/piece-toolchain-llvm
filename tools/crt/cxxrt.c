/*
 * cxxrt.c - Minimal C++ runtime stubs for S1C33 (-fno-exceptions -fno-rtti)
 *
 * P/ECE is a single-threaded environment.  Exceptions and RTTI are disabled.
 * These stubs provide the minimum set of symbols required to link C++ code.
 */

/* Abort (infinite loop — no OS to return to) */
static void __cxx_abort(void) {
    for (;;) {}
}

/*
 * Called when a pure virtual function is invoked through a vtable slot
 * that was never overridden.
 */
void __cxa_pure_virtual(void) {
    __cxx_abort();
}

/*
 * Called when a deleted virtual function is invoked.
 */
void __cxa_deleted_virtual(void) {
    __cxx_abort();
}

/*
 * Register a static destructor.
 * P/ECE applications return to the kernel on exit; atexit handlers are
 * never called.  A no-op returning success is safe.
 */
int __cxa_atexit(void (*destructor)(void *), void *arg, void *dso_handle) {
    (void)destructor;
    (void)arg;
    (void)dso_handle;
    return 0;
}

/*
 * DSO handle.  The linker references this symbol.
 * For a static executable (not a shared library), NULL is correct.
 */
void *__dso_handle = (void *)0;

/*
 * Thread-safe initialization guards for local static variables
 * (C++11 "magic statics").
 *
 * P/ECE is single-threaded, so a simple flag check suffices.
 * Guard variable layout: first byte is the "initialized" flag.
 */
int __cxa_guard_acquire(long long *guard) {
    return !*(char *)guard;  /* 1 = not yet initialized, proceed */
}

void __cxa_guard_release(long long *guard) {
    *(char *)guard = 1;  /* mark as initialized */
}

void __cxa_guard_abort(long long *guard) {
    /* Clean up after a failed initialization (exception during init).
     * Unreachable with -fno-exceptions, but the symbol is still needed. */
    (void)guard;
}

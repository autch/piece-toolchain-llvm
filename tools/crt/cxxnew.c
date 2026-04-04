/*
 * cxxnew.c - operator new/delete for S1C33
 *
 * Forwards to pceHeapAlloc()/pceHeapFree() from the P/ECE kernel API.
 * The heap is initialized by the kernel before pceAppInit00() is called.
 *
 * Compiled as C with -ffreestanding; Itanium ABI mangled names are used
 * directly since C++ operator syntax is not available.
 */

typedef unsigned long size_t;

/* P/ECE kernel API (declared directly for -ffreestanding) */
extern void *pceHeapAlloc(size_t size);
extern void pceHeapFree(void *ptr);

/* operator new(unsigned long) — _Znwm (ILP32: size_t = unsigned long = 'm') */
void *_Znwm(size_t size) {
    void *p = pceHeapAlloc(size);
    if (!p) {
        for (;;) {}  /* out of memory — no new_handler, just halt */
    }
    return p;
}

/* operator new[](unsigned long) — _Znam (ILP32: size_t = unsigned long = 'm') */
void *_Znam(size_t size) {
    return _Znwm(size);
}

/* operator delete(void*) — _ZdlPv */
void _ZdlPv(void *ptr) {
    pceHeapFree(ptr);
}

/* operator delete[](void*) — _ZdaPv */
void _ZdaPv(void *ptr) {
    pceHeapFree(ptr);
}

/* operator delete(void*, unsigned long) — _ZdlPvm (C++14 sized deallocation) */
void _ZdlPvm(void *ptr, size_t size) {
    (void)size;
    pceHeapFree(ptr);
}

/* operator delete[](void*, unsigned long) — _ZdaPvm */
void _ZdaPvm(void *ptr, size_t size) {
    (void)size;
    pceHeapFree(ptr);
}

/* nothrow variants — return NULL on failure instead of aborting */
/* operator new(unsigned long, std::nothrow_t const&) — _ZnwmRKSt9nothrow_t */
void *_ZnwmRKSt9nothrow_t(size_t size, ...) {
    return pceHeapAlloc(size);
}

/* operator new[](unsigned long, std::nothrow_t const&) — _ZnamRKSt9nothrow_t */
void *_ZnamRKSt9nothrow_t(size_t size, ...) {
    return pceHeapAlloc(size);
}

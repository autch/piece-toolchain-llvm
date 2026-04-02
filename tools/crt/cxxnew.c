/*
 * cxxnew.c - operator new/delete for S1C33
 *
 * Forwards to pceHeapAlloc()/pceHeapFree() from the P/ECE kernel API.
 * The heap is initialized by the kernel before pceAppInit00() is called.
 *
 * Compiled as C with -ffreestanding; Itanium ABI mangled names are used
 * directly since C++ operator syntax is not available.
 */

typedef unsigned int size_t;

/* P/ECE kernel API (declared directly for -ffreestanding) */
extern void *pceHeapAlloc(size_t size);
extern void pceHeapFree(void *ptr);

/* operator new(unsigned int) — _Znwj (ILP32: size_t = unsigned int = 'j') */
void *_Znwj(size_t size) {
    void *p = pceHeapAlloc(size);
    if (!p) {
        for (;;) {}  /* out of memory — no new_handler, just halt */
    }
    return p;
}

/* operator new[](unsigned int) — _Znaj */
void *_Znaj(size_t size) {
    return _Znwj(size);
}

/* operator delete(void*) — _ZdlPv */
void _ZdlPv(void *ptr) {
    pceHeapFree(ptr);
}

/* operator delete[](void*) — _ZdaPv */
void _ZdaPv(void *ptr) {
    pceHeapFree(ptr);
}

/* operator delete(void*, unsigned int) — _ZdlPvj (C++14 sized deallocation) */
void _ZdlPvj(void *ptr, size_t size) {
    (void)size;
    pceHeapFree(ptr);
}

/* operator delete[](void*, unsigned int) — _ZdaPvj */
void _ZdaPvj(void *ptr, size_t size) {
    (void)size;
    pceHeapFree(ptr);
}

/* nothrow variants — return NULL on failure instead of aborting */
/* operator new(unsigned int, std::nothrow_t const&) — _ZnwjRKSt9nothrow_t */
void *_ZnwjRKSt9nothrow_t(size_t size, ...) {
    return pceHeapAlloc(size);
}

/* operator new[](unsigned int, std::nothrow_t const&) — _ZnajRKSt9nothrow_t */
void *_ZnajRKSt9nothrow_t(size_t size, ...) {
    return pceHeapAlloc(size);
}

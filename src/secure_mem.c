#include <stddef.h>

#ifdef _DEFAULT_SOURCE
#include <sys/mman.h>
#else
#define HELIX_NO_MLOCK 1
#endif

#include "secure_mem.h"

void helix_memzero(volatile void* p, size_t n)
{
    volatile unsigned char* b = (volatile unsigned char*)p;
    while (n--) {
        *b++ = 0;
    }
}

int helix_memlock(const volatile void* p, size_t n)
{
#ifdef HELIX_NO_MLOCK
    (void)p;
    (void)n;
    return -1;
#else
    return mlock((const void*)p, n);
#endif
}

int helix_memunlock(const volatile void* p, size_t n)
{
#ifdef HELIX_NO_MLOCK
    (void)p;
    (void)n;
    return -1;
#else
    return munlock((const void*)p, n);
#endif
}

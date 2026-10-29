/* secure scrubbing for key material. a plain memset can be elided by the
   compiler when the buffer is dead afterwards; a volatile loop cannot. */
#ifndef HELIX_SECURE_MEM_H
#define HELIX_SECURE_MEM_H

#include <stddef.h>

/* scrub n bytes; safe on any alignment, never elided */
void helix_memzero(volatile void* p, size_t n);

/* best effort page pinning so secrets avoid the swap; 0 ok, -1 unsupported
   or refused (documented residual risk, see docs/limitations.md) */
int helix_memlock(const volatile void* p, size_t n);
int helix_memunlock(const volatile void* p, size_t n);

#endif

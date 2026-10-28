#ifndef HELIX_ENTROPY_H
#define HELIX_ENTROPY_H

#include <stddef.h>
#include <stdint.h>

/* fills buf from /dev/urandom. seed generation only, never in the
   derivation path. 0 ok, -1 on read failure. */
int entropy_fill(uint8_t* buf, size_t n);

#endif

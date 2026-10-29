#ifndef RANDOMBYTES_H
#define RANDOMBYTES_H

#include <stddef.h>
#include <stdint.h>

/* provided by helix (src/kyber_drbg.c), not by the kyber reference:
   a deterministic shake256 stream seeded from the master key, so a
   keypair is a pure function of the mnemonic (HX-7: the .dna file must
   decrypt from the mnemonic alone). */
void randombytes(uint8_t* out, size_t outlen);

#endif

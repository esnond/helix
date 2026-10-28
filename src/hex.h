#ifndef HELIX_HEX_H
#define HELIX_HEX_H

#include <stddef.h>
#include <stdint.h>

/* out must hold 2n + 1 bytes, always nul terminated */
void hex_encode(const uint8_t* in, size_t n, char* out);

/* 0 ok, -1 on bad hex or output too small; *out_len set on success */
int hex_decode(const char* in, uint8_t* out, size_t out_max, size_t* out_len);

#endif

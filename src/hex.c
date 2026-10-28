#include <string.h>

#include "hex.h"

static int hex_nibble(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

void hex_encode(const uint8_t* in, size_t n, char* out)
{
    static const char digits[] = "0123456789abcdef";
    size_t i;
    for (i = 0; i < n; i++) {
        out[2 * i] = digits[in[i] >> 4];
        out[2 * i + 1] = digits[in[i] & 0x0f];
    }
    out[2 * n] = '\0';
}

int hex_decode(const char* in, uint8_t* out, size_t out_max, size_t* out_len)
{
    size_t n = strlen(in);
    size_t i;
    if (n % 2 != 0 || n / 2 > out_max)
        return -1;
    for (i = 0; i < n; i += 2) {
        int hi = hex_nibble(in[i]);
        int lo = hex_nibble(in[i + 1]);
        if (hi < 0 || lo < 0)
            return -1;
        out[i / 2] = (uint8_t)((hi << 4) | lo);
    }
    *out_len = n / 2;
    return 0;
}

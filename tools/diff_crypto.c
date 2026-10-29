/* diff_crypto: batch oracle for tools/fuzz_crypto.py.
   reads length-prefixed requests from stdin, writes length-prefixed
   digests to stdout, one process for the whole run so 100k vectors cost
   no fork overhead.

   request : u8 op | u32 key_len | u32 msg_len | u32 iters | key | msg
   response: u32 out_len | out
   ops     : 0 sha256, 1 sha512, 2 hmac_sha512, 3 pbkdf2_hmac_sha512
   all integers big endian. exit 0 on clean eof. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hmac.h"
#include "pbkdf2.h"
#include "sha256.h"
#include "sha512.h"

#define MAX_FIELD 8192

static int read_exact(void* buf, size_t n)
{
    return fread(buf, 1, n, stdin) == n ? 0 : -1;
}

static uint32_t load32_be(const unsigned char* p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static void store32_be(unsigned char* p, uint32_t v)
{
    p[0] = (unsigned char)(v >> 24);
    p[1] = (unsigned char)(v >> 16);
    p[2] = (unsigned char)(v >> 8);
    p[3] = (unsigned char)v;
}

static int emit(const unsigned char* out, size_t n)
{
    unsigned char hdr[4];
    store32_be(hdr, (uint32_t)n);
    if (fwrite(hdr, 1, 4, stdout) != 4)
        return -1;
    return fwrite(out, 1, n, stdout) == n ? 0 : -1;
}

int main(void)
{
    unsigned char hdr[13];
    unsigned char key[MAX_FIELD], msg[MAX_FIELD];
    unsigned char out[256];
    for (;;) {
        uint32_t op, klen, mlen, iters;
        size_t olen;
        if (read_exact(hdr, sizeof hdr) != 0)
            break;
        op = hdr[0];
        klen = load32_be(hdr + 1);
        mlen = load32_be(hdr + 5);
        iters = load32_be(hdr + 9);
        if (klen > MAX_FIELD || mlen > MAX_FIELD)
            return 2;
        if (read_exact(key, klen) != 0 || read_exact(msg, mlen) != 0)
            return 2;
        switch (op) {
        case 0:
            sha256(msg, mlen, out);
            olen = SHA256_DIGEST;
            break;
        case 1:
            sha512(msg, mlen, out);
            olen = SHA512_DIGEST;
            break;
        case 2: {
            hmac_sha512_ctx hc;
            hmac_sha512_init(&hc, key, klen);
            hmac_sha512_update(&hc, msg, mlen);
            hmac_sha512_final(&hc, out);
            olen = SHA512_DIGEST;
            break;
        }
        case 3:
            if (iters == 0 || iters > 1000000)
                return 2;
            pbkdf2_sha512(key, klen, msg, mlen, (unsigned long)iters, out,
                          32);
            olen = 32;
            break;
        default:
            return 2;
        }
        if (emit(out, olen) != 0)
            return 2;
    }
    return fflush(stdout) == 0 ? 0 : 2;
}

/* rfc 7693 blake2b and rfc 9106 argon2id known answer tests. the
   argon2id vector is section 5.3: v 0x13, t=3, m=32 KiB, p=4 lanes,
   32 byte tag; it pins the lane indexing, the id hybrid slicing and
   the compression permutation in one shot. */
#include <stdio.h>
#include <string.h>

#include "argon2.h"
#include "blake2b.h"

static int g_fail;
static int g_ok;

static void check(int ok, const char* what)
{
    if (ok) {
        g_ok++;
        return;
    }
    g_fail++;
    printf("FAIL: %s\n", what);
}

static int hex_eq(const uint8_t* got, size_t n, const char* want)
{
    char buf[129];
    static const char hx[] = "0123456789abcdef";
    size_t i;
    if (n > 64)
        return 0;
    for (i = 0; i < n; i++) {
        buf[2 * i] = hx[got[i] >> 4];
        buf[2 * i + 1] = hx[got[i] & 15];
    }
    buf[2 * n] = 0;
    return strcmp(buf, want) == 0;
}

int main(void)
{
    uint8_t out[64];

    blake2b((const uint8_t*)"abc", 3, out, 64);
    check(hex_eq(out, 64,
                 "ba80a53f981c4d0d6a2797b69f12f6e94c212f14685ac4b74b12bb6fd"
                 "bffa2d17d87c5392aab792dc252d5de4533cc9518d38aa8dbf1925ab9"
                 "2386edd4009923"),
          "blake2b rfc 7693 abc");

    blake2b((const uint8_t*)"", 0, out, 64);
    check(hex_eq(out, 64,
                 "786a02f742015903c6c6fd852552d272912f4740e15847618a86e217f"
                 "71f5419d25e1031afee585313896444934eb04b903a685b1448b755d5"
                 "6f701afe9be2ce"),
          "blake2b empty");

    {
        uint8_t pwd[32], salt[16], sec[8], ad[12];
        memset(pwd, 0x01, sizeof pwd);
        memset(salt, 0x02, sizeof salt);
        memset(sec, 0x03, sizeof sec);
        memset(ad, 0x04, sizeof ad);
        int rc = helix_argon2id_full(pwd, sizeof pwd, salt, sizeof salt, sec,
                                     sizeof sec, ad, sizeof ad, 3, 32, 4,
                                     out, 32);
        check(rc == 0, "argon2id rfc 9106 5.3 vector");
        check(hex_eq(out, 32,
                     "0d640df58d78766c08c037a34a8b53c9d01ef0452d75b65eb5252"
                     "0e96b01e659"),
              "argon2id rfc 9106 5.3 tag");
    }

    printf("argon2id kat: %d ok, %d fail\n", g_ok, g_fail);
    return g_fail != 0;
}

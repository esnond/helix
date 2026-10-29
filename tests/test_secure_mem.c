/* secure_mem tests: the volatile scrub really zeroes, and page pinning
   degrades loudly (either 0 or a recorded -1), never silently crashes. */
#include <stdio.h>
#include <string.h>

#include "secure_mem.h"

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

int main(void)
{
    static unsigned char buf[4096];
    memset(buf, 0xa5, sizeof buf);
    helix_memzero(buf, sizeof buf);
    int clean = 1;
    for (size_t i = 0; i < sizeof buf; i++) {
        if (buf[i] != 0) {
            clean = 0;
            break;
        }
    }
    check(clean, "memzero scrubs every byte");

    /* unaligned tail */
    memset(buf, 0x5a, 17);
    helix_memzero(buf + 3, 7);
    check(buf[2] == 0x5a && buf[3] == 0 && buf[9] == 0 && buf[10] == 0x5a,
          "unaligned window scrubbed");

    int lk = helix_memlock(buf, sizeof buf);
    check(lk == 0 || lk == -1, "memlock returns 0 or -1");
    if (lk == 0) {
        check(helix_memunlock(buf, sizeof buf) == 0, "memunlock after lock");
    }

    printf("test_secure_mem: %d ok, %d failed\n", g_ok, g_fail);
    return g_fail != 0;
}

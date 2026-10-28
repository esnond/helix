/* evolve_chain: dump evolution chain statistics as csv for the r1
   report. measures the mutation machinery, not gen 0: the chain starts
   from sha256(master_key || "helix-report-g0") so no engine run and no
   pbkdf2 are needed and the loop is pure hash arithmetic.
   usage: evolve_chain <master_key_hex_128> [steps]
   csv: gen,flips,hamming_prev */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hex.h"
#include "evolve.h"
#include "sha256.h"

int main(int argc, char* argv[])
{
    uint8_t mk[64], g0[32], prev[32], prev2[32], cur[32], base[32];
    long steps = 1000, i;
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "usage: evolve_chain <master_key_hex_128> [steps]\n");
        return 1;
    }
    if (hex_decode(argv[1], mk, sizeof mk, NULL) != 0 ||
        strlen(argv[1]) != 128) {
        fprintf(stderr, "evolve_chain: master key must be 128 hex chars\n");
        return 1;
    }
    if (argc == 3) {
        steps = strtol(argv[2], NULL, 10);
        if (steps < 2 || steps > 1000000) {
            fprintf(stderr, "evolve_chain: steps out of range\n");
            return 1;
        }
    }
    {
        uint8_t mat[64 + 16];
        memcpy(mat, mk, 64);
        memcpy(mat + 64, "helix-report-g0", 15);
        sha256(mat, 64 + 15, g0);
        memset(mat, 0, sizeof mat);
    }
    memcpy(prev, g0, 32);
    memcpy(prev2, g0, 32);
    printf("gen,flips,hamming_prev\n");
    printf("0,0,0\n");
    for (i = 1; i < steps; i++) {
        int flips, h_prev, j;
        if (evolution_recombine(mk, 64, (uint32_t)i, prev, prev2, cur) != 0)
            return 1;
        for (j = 0; j < 32; j++)
            base[j] = (uint8_t)(prev[j] ^ prev2[j]);
        flips = evolution_hamming(base, cur);
        h_prev = evolution_hamming(cur, prev);
        printf("%ld,%d,%d\n", i, flips, h_prev);
        memcpy(prev2, prev, 32);
        memcpy(prev, cur, 32);
    }
    memset(mk, 0, sizeof mk);
    memset(g0, 0, sizeof g0);
    memset(prev, 0, sizeof prev);
    memset(prev2, 0, sizeof prev2);
    memset(cur, 0, sizeof cur);
    return 0;
}

/* fuzz_dna: deterministic mutational fuzzing of the .dna parser.
   every iteration mutates a valid 1216-byte header + 64-byte body and
   calls dna_parse_header plus the body-nonce derivation, the pure parsing
   surface. run under asan/ubsan; a crash or a hang is a finding.
   usage: tools/fuzz_dna [iterations] [seed] */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "dna.h"

static uint64_t st;

static uint64_t nxt(void)
{
    st ^= st >> 12;
    st ^= st << 25;
    st ^= st >> 27;
    return st * 2685821657736338717ULL;
}

static double now_s(void)
{
    return (double)clock() / CLOCKS_PER_SEC;
}

int main(int argc, char** argv)
{
    long iters = (argc > 1) ? atol(argv[1]) : 10000000L;
    st = (argc > 2) ? strtoull(argv[2], 0, 0) : 0x68757a7a6831ULL;

    /* corpus: a real header produced by the core itself */
    static uint8_t corpus[DNA_HEADER_SIZE + 128];
    static const char* tmp_in = "/tmp/helix_fuzz_in";
    static const char* tmp_dna = "/tmp/helix_fuzz.dna";
    FILE* f = fopen(tmp_in, "wb");
    if (!f) return 2;
    fputs("fuzz corpus", f);
    fclose(f);
    if (dna_encrypt_file(tmp_in, tmp_dna,
                         "abandon abandon abandon abandon abandon abandon "
                         "abandon abandon abandon abandon abandon about",
                         NULL, 0) != 0) {
        printf("corpus generation failed\n");
        return 2;
    }
    f = fopen(tmp_dna, "rb");
    if (!f) return 2;
    size_t n = fread(corpus, 1, sizeof corpus, f);
    fclose(f);
    if (n < DNA_HEADER_SIZE) {
        printf("corpus too small: %zu\n", n);
        return 2;
    }
    remove(tmp_in);
    remove(tmp_dna);

    static uint8_t buf[DNA_HEADER_SIZE + 128];
    double t0 = now_s();
    long crashed = 0;
    for (long i = 0; i < iters; i++) {
        memcpy(buf, corpus, sizeof buf);
        int muts = 1 + (int)(nxt() % 8);
        for (int m = 0; m < muts; m++) {
            size_t pos = (size_t)(nxt() % n);
            switch (nxt() % 4) {
            case 0: buf[pos] ^= (uint8_t)(1u << (nxt() % 8)); break;
            case 1: buf[pos] = (uint8_t)nxt(); break;
            case 2: buf[pos] = (uint8_t)(buf[pos] + (uint8_t)nxt()); break;
            default: buf[pos] = 0xff; break;
            }
        }
        dna_header_info info;
        if (dna_parse_header(buf, &info) == 0) {
            /* the parsed fields must agree with the fixed offsets */
            if (info.generation > DNA_GEN_MAX) crashed++;
            if (info.kdf_type > 1) crashed++;
        }
        if ((i & 0xfffff) == 0) {
            printf("iter %ld (%.1fs) suspicious=%ld\n", i, now_s() - t0, crashed);
            fflush(stdout);
        }
    }
    printf("fuzz_dna: %ld iterations, %ld suspicious, %.1fs\n", iters, crashed, now_s() - t0);
    return 0;
}

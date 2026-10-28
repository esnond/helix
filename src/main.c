/* helix cli: seed generation and key derivation. r0 skeleton. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "helix_config.h"
#include "hex.h"
#include "bip39.h"
#include "entropy.h"
#include "kdf.h"

static void usage(FILE* out)
{
    fprintf(out,
        "usage: helix <command> [flags]\n"
        "\n"
        "  gen-seed [-w 12|24] [-e]\n"
        "      print a fresh bip39 mnemonic (default 24 words),\n"
        "      -e also prints the raw entropy hex\n"
        "\n"
        "  derive -m \"word word ...\" [-s salt_hex] [-c connectome.csv]\n"
        "      derive the 32 byte key from a mnemonic, print hex;\n"
        "      salt defaults to empty, stats go to stderr\n");
}

static int cmd_gen_seed(int argc, char* argv[])
{
    uint8_t entropy[BIP39_ENTROPY_MAX];
    char mnemonic[BIP39_MNEMONIC_MAX];
    char hex[2 * BIP39_ENTROPY_MAX + 1];
    size_t ent_len;
    int words = 24, show_entropy = 0, opt;
    while ((opt = getopt(argc, argv, "w:e")) != -1) {
        switch (opt) {
        case 'w':
            words = atoi(optarg);
            break;
        case 'e':
            show_entropy = 1;
            break;
        default:
            return 1;
        }
    }
    if (words != 12 && words != 24) {
        fprintf(stderr, "helix: word count must be 12 or 24\n");
        return 1;
    }
    ent_len = (words == 12) ? 16 : 32;
    if (entropy_fill(entropy, ent_len) != 0) {
        fprintf(stderr, "helix: cannot read /dev/urandom\n");
        return 1;
    }
    if (bip39_entropy_to_mnemonic(entropy, ent_len, mnemonic,
                                  sizeof mnemonic) != 0) {
        fprintf(stderr, "helix: mnemonic encoding failed\n");
        return 1;
    }
    printf("%s\n", mnemonic);
    if (show_entropy) {
        hex_encode(entropy, ent_len, hex);
        printf("entropy: %s\n", hex);
    }
    memset(entropy, 0, sizeof entropy);
    return 0;
}

static int cmd_derive(int argc, char* argv[])
{
    uint8_t salt[64];
    uint8_t key[HELIX_DERIVED_KEY_LEN];
    size_t salt_len = 0;
    helix_stats st;
    char out[2 * HELIX_DERIVED_KEY_LEN + 1];
    const char* mnemonic = NULL;
    const char* connectome = NULL;
    const char* salt_hex = "";
    int opt, rc;
    while ((opt = getopt(argc, argv, "m:s:c:")) != -1) {
        switch (opt) {
        case 'm':
            mnemonic = optarg;
            break;
        case 's':
            salt_hex = optarg;
            break;
        case 'c':
            connectome = optarg;
            break;
        default:
            return 1;
        }
    }
    if (!mnemonic) {
        usage(stderr);
        return 1;
    }
    if (hex_decode(salt_hex, salt, sizeof salt, &salt_len) != 0) {
        fprintf(stderr, "helix: bad salt hex\n");
        return 1;
    }
    rc = helix_derive(mnemonic, salt, salt_len, connectome, key, &st);
    if (rc == -1) {
        fprintf(stderr, "helix: invalid mnemonic\n");
        return 2;
    }
    if (rc == -2) {
        fprintf(stderr, "helix: connectome init failed\n");
        return 3;
    }
    hex_encode(key, HELIX_DERIVED_KEY_LEN, out);
    printf("%s\n", out);
    fprintf(stderr, "helix: %d cells, %lu iters, %ld steps\n",
            st.n_cells, st.iters, st.steps);
    return 0;
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        usage(stderr);
        return 1;
    }
    if (strcmp(argv[1], "gen-seed") == 0)
        return cmd_gen_seed(argc - 1, argv + 1);
    if (strcmp(argv[1], "derive") == 0)
        return cmd_derive(argc - 1, argv + 1);
    usage(stderr);
    return 1;
}

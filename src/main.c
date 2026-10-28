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
        "           [-g generation] [-k keychain.db]\n"
        "      derive the 32 byte key from a mnemonic, print hex;\n"
        "      salt defaults to empty, stats go to stderr;\n"
        "      -g fetches an existing generation from the keychain\n"
        "      instead of deriving (stateless gen 0 without -g)\n"
        "\n"
        "  evolve -m \"word word ...\" [-k keychain.db]\n"
        "      advance the key evolution chain one generation\n"
        "      (bootstraps gen 0 and gen 1 on first use) and print\n"
        "      the new generation number and key hex\n");
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
    const char* kch = NULL;
    uint32_t want_gen = 0;
    int opt, rc, have_gen = 0;
    while ((opt = getopt(argc, argv, "m:s:c:g:k:")) != -1) {
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
        case 'g':
            want_gen = (uint32_t)strtoul(optarg, NULL, 10);
            have_gen = 1;
            break;
        case 'k':
            kch = optarg;
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
    if (have_gen) {
        rc = helix_derive_generation(mnemonic, kch, want_gen, key);
    } else {
        rc = helix_derive(mnemonic, salt, salt_len, connectome, key, &st);
        fprintf(stderr, "helix: %d cells, %lu iters, %ld steps\n",
                st.n_cells, st.iters, st.steps);
    }
    if (rc == -1) {
        fprintf(stderr, "helix: invalid mnemonic\n");
        return 2;
    }
    if (rc == -2) {
        fprintf(stderr, "helix: connectome init failed\n");
        return 3;
    }
    if (rc == -3 || rc == -4) {
        fprintf(stderr, "helix: keychain error (%d)\n", rc);
        return 4;
    }
    hex_encode(key, HELIX_DERIVED_KEY_LEN, out);
    printf("%s\n", out);
    if (have_gen)
        fprintf(stderr, "helix: generation %u from keychain\n", want_gen);
    return 0;
}

static int cmd_evolve(int argc, char* argv[])
{
    uint8_t key[HELIX_DERIVED_KEY_LEN];
    helix_stats st;
    char out[2 * HELIX_DERIVED_KEY_LEN + 1];
    const char* mnemonic = NULL;
    const char* kch = NULL;
    uint32_t gen = 0;
    int opt, rc;
    while ((opt = getopt(argc, argv, "m:k:")) != -1) {
        switch (opt) {
        case 'm':
            mnemonic = optarg;
            break;
        case 'k':
            kch = optarg;
            break;
        default:
            return 1;
        }
    }
    if (!mnemonic) {
        usage(stderr);
        return 1;
    }
    rc = helix_evolve(mnemonic, kch, &gen, key, &st);
    if (rc == -1) {
        fprintf(stderr, "helix: invalid mnemonic\n");
        return 2;
    }
    if (rc == -2) {
        fprintf(stderr, "helix: connectome init failed\n");
        return 3;
    }
    if (rc == -3 || rc == -4) {
        fprintf(stderr, "helix: keychain error (%d)\n", rc);
        return 4;
    }
    hex_encode(key, HELIX_DERIVED_KEY_LEN, out);
    printf("%u %s\n", gen, out);
    if (st.n_cells > 0)
        fprintf(stderr, "helix: bootstrapped gen 0 (%d cells)\n", st.n_cells);
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
    if (strcmp(argv[1], "evolve") == 0)
        return cmd_evolve(argc - 1, argv + 1);
    usage(stderr);
    return 1;
}

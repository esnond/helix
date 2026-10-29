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
#include "secure_mem.h"
#include "keychain.h"
#include "dna.h"
#include "hkpair.h"
#include "kdf.h"
#include "secure_mem.h"

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
        "      the new generation number and key hex\n"
        "\n"
        "  encrypt <in> <out.dna> -m \"word word ...\" [-k keychain.db]\n"
        "           [-g generation]\n"
        "      encrypt a file into the .dna archive format; the\n"
        "      generation defaults to the current keychain generation\n"
        "      (or 0 without a keychain)\n"
        "\n"
        "  decrypt <in.dna> <out> -m \"word word ...\" [-k keychain.db]\n"
        "      decrypt a .dna archive; the generation key is recomputed\n"
        "      from the mnemonic, the keychain is only a cache\n"
        "\n"
        "  pair create <file.hkpair> -p passphrase [-k pbkdf2|argon2id]\n"
        "      create an external kyber-768 keypair for v3 .dna files;\n"
        "      kdf defaults to argon2id. decryption then needs this file\n"
        "      AND the mnemonic; the mnemonic alone fails\n"
        "\n"
        "  pair export <file.hkpair> <out.pub> -p passphrase\n"
        "      write the raw 1184-byte public key for sharing\n"
        "\n"
        "  migrate-kch -m \"word word ...\" [-k keychain.db]\n"
        "      convert a legacy kch1 keychain to the encrypted kch2\n"
        "      format in place; happens automatically on the first\n"
        "      evolve otherwise\n");
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

static int cmd_encrypt(int argc, char* argv[])
{
    const char* mnemonic = NULL;
    const char* kch = NULL;
    const char* in = NULL;
    const char* out = NULL;
    const char* pair = NULL;
    const char* pair_pass = NULL;
    uint32_t gen = 0;
    int have_gen = 0, opt;
    while ((opt = getopt(argc, argv, "m:k:g:P:p:")) != -1) {
        switch (opt) {
        case 'm':
            mnemonic = optarg;
            break;
        case 'k':
            kch = optarg;
            break;
        case 'g':
            gen = (uint32_t)strtoul(optarg, NULL, 10);
            have_gen = 1;
            break;
        case 'P':
            pair = optarg;
            break;
        case 'p':
            pair_pass = optarg;
            break;
        default:
            return 1;
        }
    }
    if (argc - optind != 2 || !mnemonic || (pair && !pair_pass)) {
        usage(stderr);
        return 1;
    }
    in = argv[optind];
    out = argv[optind + 1];
    if (!have_gen) {
        /* default generation: current keychain generation if a
           keychain exists, else gen 0 (documented state dependency) */
        if (helix_current_generation(mnemonic, kch, &gen) != 0)
            gen = 0;
    }
    switch (dna_encrypt_file_ex(in, out, mnemonic, kch, gen, pair,
                                pair_pass)) {
    case 0:
        fprintf(stderr, "helix: encrypted %s -> %s (generation %u%s)\n",
                in, out, gen, pair ? ", external pair" : "");
        return 0;
    case -3:
        fprintf(stderr, "helix: engine/keychain failure (connectome?)\n");
        return 4;
    case -7:
        fprintf(stderr, "helix: invalid mnemonic\n");
        return 2;
    case -4:
        fprintf(stderr, "helix: pair passphrase rejected\n");
        return 5;
    case -5:
        fprintf(stderr, "helix: out of memory\n");
        return 5;
    case -6:
        fprintf(stderr, "helix: %s already exists\n", out);
        return 6;
    default:
        fprintf(stderr, "helix: encrypt failed (io error)\n");
        return 1;
    }
}

static int cmd_decrypt(int argc, char* argv[])
{
    const char* mnemonic = NULL;
    const char* kch = NULL;
    const char* in = NULL;
    const char* out = NULL;
    const char* pair = NULL;
    const char* pair_pass = NULL;
    int opt;
    while ((opt = getopt(argc, argv, "m:k:P:p:")) != -1) {
        switch (opt) {
        case 'm':
            mnemonic = optarg;
            break;
        case 'k':
            kch = optarg;
            break;
        case 'P':
            pair = optarg;
            break;
        case 'p':
            pair_pass = optarg;
            break;
        default:
            return 1;
        }
    }
    if (argc - optind != 2 || !mnemonic || (pair && !pair_pass)) {
        usage(stderr);
        return 1;
    }
    in = argv[optind];
    out = argv[optind + 1];
    switch (dna_decrypt_file_ex(in, out, mnemonic, kch, pair, pair_pass)) {
    case 0:
        fprintf(stderr, "helix: decrypted %s -> %s\n", in, out);
        return 0;
    case -2:
        fprintf(stderr, "helix: not a valid .dna file\n");
        return 2;
    case -3:
        fprintf(stderr, "helix: generation key unavailable\n");
        return 4;
    case -4:
        fprintf(stderr, "helix: authentication failed "
                        "(wrong mnemonic or corrupted file)\n");
        return 5;
    default:
        fprintf(stderr, "helix: decrypt failed (io error)\n");
        return 1;
    }
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

static int cmd_migrate_kch(int argc, char* argv[])
{
    const char* mnemonic = NULL;
    const char* kch = NULL;
    uint8_t mk[HELIX_MASTER_KEY_LEN];
    char path[4096];
    keychain_t kc;
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
    if (argc - optind != 0 || !mnemonic) {
        usage(stderr);
        return 1;
    }
    if (helix_master_key_from_mnemonic(mnemonic, NULL, 0, mk) != 0) {
        fprintf(stderr, "helix: bad mnemonic\n");
        return 2;
    }
    if (kch) {
        if (strlen(kch) >= sizeof path) {
            fprintf(stderr, "helix: path too long\n");
            return 1;
        }
        strcpy(path, kch);
    } else if (helix_keychain_path(mk, path, sizeof path) != 0) {
        fprintf(stderr, "helix: HOME unset\n");
        return 1;
    }
    rc = keychain_open(&kc, path, mk);
    if (rc != 0) {
        fprintf(stderr, "helix: keychain open failed (%d)\n", rc);
        helix_memzero(mk, sizeof mk);
        return 2;
    }
    rc = keychain_migrate(&kc);
    if (rc == 0)
        fprintf(stderr, "helix: keychain at %s is kch2 (%u generations)\n",
                path, kc.n_entries);
    else
        fprintf(stderr, "helix: migration failed (%d)\n", rc);
    keychain_close(&kc);
    helix_memzero(mk, sizeof mk);
    return rc == 0 ? 0 : 2;
}

static int cmd_pair_create(int argc, char* argv[])
{
    const char* pass = NULL;
    const char* out = NULL;
    int kdf = HKP_KDF_ARGON2ID, opt;
    while ((opt = getopt(argc, argv, "p:k:")) != -1) {
        switch (opt) {
        case 'p':
            pass = optarg;
            break;
        case 'k':
            if (strcmp(optarg, "argon2id") == 0)
                kdf = HKP_KDF_ARGON2ID;
            else if (strcmp(optarg, "pbkdf2") == 0)
                kdf = HKP_KDF_PBKDF2;
            else {
                fprintf(stderr, "helix: unknown kdf %s\n", optarg);
                return 2;
            }
            break;
        default:
            return 1;
        }
    }
    if (argc - optind != 1 || !pass || !pass[0]) {
        usage(stderr);
        return 1;
    }
    out = argv[optind];
    switch (hkpair_create(out, pass, kdf)) {
    case 0:
        fprintf(stderr, "helix: pair created: %s (%s)\n", out,
                kdf == HKP_KDF_ARGON2ID ? "argon2id" : "pbkdf2");
        return 0;
    case -6:
        fprintf(stderr, "helix: %s already exists\n", out);
        return 6;
    case -1:
        fprintf(stderr, "helix: io error\n");
        return 1;
    default:
        fprintf(stderr, "helix: pair creation failed\n");
        return 2;
    }
}

static int cmd_pair_export(int argc, char* argv[])
{
    const char* pass = NULL;
    int opt;
    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
        case 'p':
            pass = optarg;
            break;
        default:
            return 1;
        }
    }
    if (argc - optind != 2 || !pass) {
        usage(stderr);
        return 1;
    }
    switch (hkpair_export_pk(argv[optind], pass, argv[optind + 1])) {
    case 0:
        fprintf(stderr, "helix: public key written: %s\n",
                argv[optind + 1]);
        return 0;
    case -4:
        fprintf(stderr, "helix: pair passphrase rejected\n");
        return 5;
    default:
        fprintf(stderr, "helix: pair export failed\n");
        return 1;
    }
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        usage(stderr);
        return 1;
    }
    if (strcmp(argv[1], "gen-seed") == 0) {
        optind = 1;
        return cmd_gen_seed(argc - 1, argv + 1);
    }
    if (strcmp(argv[1], "derive") == 0) {
        optind = 1;
        return cmd_derive(argc - 1, argv + 1);
    }
    if (strcmp(argv[1], "evolve") == 0) {
        optind = 1;
        return cmd_evolve(argc - 1, argv + 1);
    }
    if (strcmp(argv[1], "encrypt") == 0) {
        optind = 1;
        return cmd_encrypt(argc - 1, argv + 1);
    }
    if (strcmp(argv[1], "decrypt") == 0) {
        optind = 1;
        return cmd_decrypt(argc - 1, argv + 1);
    }
    if (strcmp(argv[1], "migrate-kch") == 0) {
        optind = 1;
        return cmd_migrate_kch(argc - 1, argv + 1);
    }
    if (strcmp(argv[1], "pair") == 0 && argc >= 3 &&
        strcmp(argv[2], "create") == 0) {
        optind = 1;
        return cmd_pair_create(argc - 2, argv + 2);
    }
    if (strcmp(argv[1], "pair") == 0 && argc >= 3 &&
        strcmp(argv[2], "export") == 0) {
        optind = 1;
        return cmd_pair_export(argc - 2, argv + 2);
    }
    usage(stderr);
    return 1;
}

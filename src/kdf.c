/* helix key derivation: bip39 entropy -> pbkdf2 master key -> petri
   connectome trajectory -> sha256 over quantized activations.
   every input is fixed by the mnemonic and salt; the engine's own
   determinism (xorshift64*, fixed fp order) carries bitwise replay. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "helix_config.h"
#include "secure_mem.h"
#include "kdf.h"
#include "bip39.h"
#include "pbkdf2.h"
#include "sha256.h"
#include "hex.h"
#include "keychain.h"
#include "evolve.h"
#include "neuron.h"
#include "sim.h"

void helix_master_key(const uint8_t* entropy, size_t ent_len,
                      const uint8_t* salt, size_t salt_len,
                      uint8_t mk[HELIX_MASTER_KEY_LEN])
{
    pbkdf2_sha512(entropy, ent_len, salt, salt_len,
                  HELIX_PBKDF2_ITERS, mk, HELIX_MASTER_KEY_LEN);
}

int helix_master_key_from_mnemonic(const char* mnemonic,
                                   const uint8_t* salt, size_t salt_len,
                                   uint8_t mk[HELIX_MASTER_KEY_LEN])
{
    uint8_t entropy[BIP39_ENTROPY_MAX];
    size_t ent_len = 0;
    if (bip39_mnemonic_to_entropy(mnemonic, entropy, sizeof entropy,
                                  &ent_len) != 0)
        return -1;
    helix_master_key(entropy, ent_len, salt, salt_len, mk);
    helix_memzero(entropy, sizeof entropy);
    return 0;
}

/* rng seed: first 8 master key bytes, big endian */
static uint64_t key_seed(const uint8_t* mk)
{
    uint64_t s = 0;
    int i;
    for (i = 0; i < 8; i++)
        s = (s << 8) | (uint64_t)mk[i];
    return s;
}

static int petri_init(sim_t* sim, const uint8_t* mk, const char* connectome)
{
    sim_opts_t opts;
    int i;
    helix_memzero(sim, sizeof *sim);
    memset(&opts, 0, sizeof opts);
    opts.duration_s = (double)HELIX_KDF_STEPS;   /* ms, advisory only */
    opts.seed = key_seed(mk);
    opts.headless = 1;
    opts.quiet = 1;
    opts.connectome_path = connectome;
    if (sim_init(sim, &opts) != 0)
        return -1;
    /* master key bytes 8..63 mapped into initial membrane state:
       signed byte scaled to +/- HELIX_INIT_SPREAD_MV, cycling cells */
    for (i = 0; i < sim->ns.n; i++) {
        uint8_t kb = mk[8 + (size_t)i % (HELIX_MASTER_KEY_LEN - 8)];
        sim->ns.cells[i].v +=
            ((float)(int8_t)kb / 128.0f) * HELIX_INIT_SPREAD_MV;
    }
    return 0;
}

int helix_engine_phase(const uint8_t mk[HELIX_MASTER_KEY_LEN],
                       const char* connectome,
                       uint8_t out[HELIX_DERIVED_KEY_LEN], helix_stats* stats)
{
    static sim_t sim;   /* roughly 1 mb, kept off the stack */
    uint8_t acts[MAX_NEURONS];
    sha256_ctx hc;
    long step;
    int i, n_cells, rc = 0;
    {
        const char* env = getenv("HELIX_CONNECTOME");
        const char* path = connectome;
        if (!path)
            path = (env && *env) ? env : HELIX_CONNECTOME_PATH;
        if (petri_init(&sim, mk, path) != 0)
            return -2;
    }
    n_cells = sim.ns.n;
    sha256_init(&hc);
    for (step = 0; step < HELIX_KDF_STEPS; step++) {
        sim_step(&sim);
        for (i = 0; i < n_cells; i++)
            acts[i] = (uint8_t)(neuron_act(&sim.ns.cells[i]) * 255.0f + 0.5f);
        sha256_update(&hc, acts, (size_t)n_cells);
    }
    /* the trajectory is contractive under a fixed noise stream, so the
       initial membrane offsets (master key bytes 8..63) can decay away;
       folding the full master key in keeps every key byte load bearing */
    sha256_update(&hc, mk, HELIX_MASTER_KEY_LEN);
    sha256_final(&hc, out);
    if (stats) {
        stats->n_cells = n_cells;
        stats->iters = HELIX_PBKDF2_ITERS;
        stats->steps = HELIX_KDF_STEPS;
    }
    helix_memzero(acts, sizeof acts);
    helix_memzero(&hc, sizeof hc);
    helix_memzero(&sim, sizeof sim);
    return rc;
}

int helix_derive(const char* mnemonic,
                 const uint8_t* salt, size_t salt_len,
                 const char* connectome,
                 uint8_t out[HELIX_DERIVED_KEY_LEN], helix_stats* stats)
{
    uint8_t mk[HELIX_MASTER_KEY_LEN];
    int rc;
    if (helix_master_key_from_mnemonic(mnemonic, salt, salt_len, mk) != 0)
        return -1;
    rc = helix_engine_phase(mk, connectome, out, stats);
    helix_memzero(mk, sizeof mk);
    return rc;
}

/* keychain fingerprint: first 8 bytes of sha256(master key) */
static void master_fp(const uint8_t* mk, uint8_t fp[KEYCHAIN_FP_LEN])
{
    uint8_t digest[SHA256_DIGEST];
    sha256(mk, HELIX_MASTER_KEY_LEN, digest);
    memcpy(fp, digest, KEYCHAIN_FP_LEN);
    helix_memzero(digest, sizeof digest);
}

int helix_keychain_path(const uint8_t mk[HELIX_MASTER_KEY_LEN],
                        char* out, size_t out_size)
{
    const char* home = getenv("HOME");
    uint8_t fp[KEYCHAIN_FP_LEN];
    char hex[2 * KEYCHAIN_FP_LEN + 1];
    if (!home || !*home)
        return -1;
    master_fp(mk, fp);
    hex_encode(fp, KEYCHAIN_FP_LEN, hex);
    snprintf(out, out_size, "%s/.helix-kch-%s.db", home, hex);
    return 0;
}

int helix_evolve(const char* mnemonic, const char* keychain_path,
                 uint32_t* gen_out, uint8_t key[HELIX_DERIVED_KEY_LEN],
                 helix_stats* stats)
{
    uint8_t mk[HELIX_MASTER_KEY_LEN], fp[KEYCHAIN_FP_LEN];
    uint8_t gp[32], gpp[32];
    char path[4096];
    keychain_t kc;
    uint32_t next;
    int rc;
    if (helix_master_key_from_mnemonic(mnemonic, NULL, 0, mk) != 0)
        return -1;
    master_fp(mk, fp);
    if (keychain_path) {
        if (strlen(keychain_path) >= sizeof path) {
            helix_memzero(mk, sizeof mk);
            return -3;
        }
        strcpy(path, keychain_path);
    } else if (helix_keychain_path(mk, path, sizeof path) != 0) {
        helix_memzero(mk, sizeof mk);
        return -4;
    }
    rc = keychain_open(&kc, path, fp);
    if (rc != 0) {
        helix_memzero(mk, sizeof mk);
        return -3;
    }
    if (kc.n_entries == 0) {
        /* bootstrap: gen 0 is the stateless r0 derivation, empty salt;
           gen 1 = recombine(gen 0, gen 0) */
        rc = helix_engine_phase(mk, NULL, gp, stats);
        if (rc != 0)
            goto out;
        if (keychain_append(&kc, gp) != 0) {
            rc = -3;
            goto out;
        }
        next = 1;
    } else {
        next = kc.n_entries;
        /* a crashed bootstrap may leave exactly gen 0; heal by
           appending gen 1 from it */
        if (next < 1 || keychain_get(&kc, next - 1, gp) != 0 ||
            (next >= 2 && keychain_get(&kc, next - 2, gpp) != 0)) {
            rc = -3;
            goto out;
        }
    }
    if (next >= 1) {
        /* for next == 1 the parents are gen 0 twice */
        uint8_t parent_b[32];
        if (next == 1)
            memcpy(parent_b, gp, 32);
        else
            memcpy(parent_b, gpp, 32);
        if (evolution_recombine(mk, HELIX_MASTER_KEY_LEN, next,
                                gp, parent_b, key) != 0) {
            rc = -3;
            goto out;
        }
        if (keychain_append(&kc, key) != 0) {
            rc = -3;
            goto out;
        }
    }
    if (gen_out)
        *gen_out = next;
out:
    keychain_close(&kc);
    helix_memzero(mk, sizeof mk);
    helix_memzero(gp, sizeof gp);
    helix_memzero(gpp, sizeof gpp);
    return rc;
}

int helix_derive_generation(const char* mnemonic, const char* keychain_path,
                            uint32_t gen,
                            uint8_t key[HELIX_DERIVED_KEY_LEN])
{
    uint8_t mk[HELIX_MASTER_KEY_LEN], fp[KEYCHAIN_FP_LEN];
    char path[4096];
    keychain_t kc;
    int rc;
    if (helix_master_key_from_mnemonic(mnemonic, NULL, 0, mk) != 0)
        return -1;
    master_fp(mk, fp);
    if (keychain_path) {
        if (strlen(keychain_path) >= sizeof path) {
            helix_memzero(mk, sizeof mk);
            return -3;
        }
        strcpy(path, keychain_path);
    } else if (helix_keychain_path(mk, path, sizeof path) != 0) {
        helix_memzero(mk, sizeof mk);
        return -4;
    }
    rc = keychain_open(&kc, path, fp);
    if (rc != 0) {
        helix_memzero(mk, sizeof mk);
        return -3;
    }
    if (keychain_get(&kc, gen, key) != 0)
        rc = -3;
    keychain_close(&kc);
    helix_memzero(mk, sizeof mk);
    return rc;
}

int helix_current_generation(const char* mnemonic, const char* keychain_path,
                             uint32_t* gen)
{
    uint8_t mk[HELIX_MASTER_KEY_LEN], fp[KEYCHAIN_FP_LEN];
    char path[4096];
    keychain_t kc;
    *gen = 0;
    if (helix_master_key_from_mnemonic(mnemonic, NULL, 0, mk) != 0)
        return -1;
    master_fp(mk, fp);
    if (keychain_path) {
        if (strlen(keychain_path) < sizeof path)
            strcpy(path, keychain_path);
        else
            path[0] = '\0';
    } else if (helix_keychain_path(mk, path, sizeof path) != 0) {
        path[0] = '\0';   /* no HOME: treat as no keychain */
    }
    if (path[0] != '\0' && keychain_open(&kc, path, fp) == 0) {
        if (kc.n_entries > 0)
            *gen = kc.current_gen;
        keychain_close(&kc);
    }
    helix_memzero(mk, sizeof mk);
    return 0;
}

int helix_generation_key(const char* mnemonic, const char* keychain_path,
                         uint32_t gen, uint8_t key[HELIX_DERIVED_KEY_LEN])
{
    uint8_t mk[HELIX_MASTER_KEY_LEN], fp[KEYCHAIN_FP_LEN];
    uint8_t g0[32], prev[32], prev2[32], cur[32];
    char path[4096];
    keychain_t kc;
    uint32_t n;
    int rc, have_cache = 0;
    if (helix_master_key_from_mnemonic(mnemonic, NULL, 0, mk) != 0)
        return -1;
    master_fp(mk, fp);
    if (keychain_path && strlen(keychain_path) < sizeof path) {
        strcpy(path, keychain_path);
        if (keychain_open(&kc, path, fp) == 0) {
            have_cache = (keychain_get(&kc, gen, key) == 0);
            keychain_close(&kc);
        }
    }
    if (have_cache) {
        helix_memzero(mk, sizeof mk);
        return 0;
    }
    /* recompute the chain: gen 0 is the engine phase, gen n the
       recombination of its two parents, exactly as helix_evolve */
    rc = helix_engine_phase(mk, NULL, g0, NULL);
    if (rc != 0) {
        helix_memzero(mk, sizeof mk);
        return -2;
    }
    if (gen == 0) {
        memcpy(key, g0, 32);
    } else {
        memcpy(prev, g0, 32);
        memcpy(prev2, g0, 32);
        for (n = 1; n <= gen; n++) {
            if (evolution_recombine(mk, HELIX_MASTER_KEY_LEN, n,
                                    prev, prev2, cur) != 0) {
                helix_memzero(mk, sizeof mk);
                return -2;
            }
            memcpy(prev2, prev, 32);
            memcpy(prev, cur, 32);
        }
        memcpy(key, prev, 32);
    }
    helix_memzero(mk, sizeof mk);
    helix_memzero(g0, sizeof g0);
    helix_memzero(prev, sizeof prev);
    helix_memzero(prev2, sizeof prev2);
    helix_memzero(cur, sizeof cur);
    return 0;
}

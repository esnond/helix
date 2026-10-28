/* helix key derivation: bip39 entropy -> pbkdf2 master key -> petri
   connectome trajectory -> sha256 over quantized activations.
   every input is fixed by the mnemonic and salt; the engine's own
   determinism (xorshift64*, fixed fp order) carries bitwise replay. */
#include <string.h>

#include "helix_config.h"
#include "kdf.h"
#include "bip39.h"
#include "pbkdf2.h"
#include "sha256.h"
#include "neuron.h"
#include "sim.h"

void helix_master_key(const uint8_t* entropy, size_t ent_len,
                      const uint8_t* salt, size_t salt_len,
                      uint8_t mk[HELIX_MASTER_KEY_LEN])
{
    pbkdf2_sha512(entropy, ent_len, salt, salt_len,
                  HELIX_PBKDF2_ITERS, mk, HELIX_MASTER_KEY_LEN);
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
    memset(sim, 0, sizeof *sim);
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

int helix_derive(const char* mnemonic,
                 const uint8_t* salt, size_t salt_len,
                 const char* connectome,
                 uint8_t out[HELIX_DERIVED_KEY_LEN], helix_stats* stats)
{
    static sim_t sim;   /* roughly 1 mb, kept off the stack */
    uint8_t entropy[BIP39_ENTROPY_MAX];
    uint8_t mk[HELIX_MASTER_KEY_LEN];
    uint8_t acts[MAX_NEURONS];
    sha256_ctx hc;
    size_t ent_len = 0;
    long step;
    int i, n_cells, rc = 0;
    if (bip39_mnemonic_to_entropy(mnemonic, entropy, sizeof entropy,
                                  &ent_len) != 0)
        return -1;
    helix_master_key(entropy, ent_len, salt, salt_len, mk);
    memset(entropy, 0, sizeof entropy);
    if (petri_init(&sim, mk,
                   connectome ? connectome : HELIX_CONNECTOME_PATH) != 0) {
        memset(mk, 0, sizeof mk);
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
    memset(mk, 0, sizeof mk);
    memset(acts, 0, sizeof acts);
    memset(&hc, 0, sizeof hc);
    memset(&sim, 0, sizeof sim);
    return rc;
}

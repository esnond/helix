/* gui_api: see gui_api.h. every function is a straight call into an
   existing helix module; the rust side owns all buffers. */
#include <string.h>

#include "gui_api.h"
#include "bip39.h"
#include "entropy.h"
#include "dna.h"
#include "kdf.h"
#include "keychain.h"

int helix_gui_gen_seed(unsigned words, char* out_mnemonic,
                       size_t out_size, unsigned char* out_entropy,
                       size_t entropy_len)
{
    size_t ent_len;
    if ((words != 12 && words != 24) || entropy_len != 32)
        return -1;
    ent_len = (words == 12) ? 16 : 32;
    if (entropy_fill(out_entropy, ent_len) != 0)
        return -1;
    if (bip39_entropy_to_mnemonic(out_entropy, ent_len, out_mnemonic,
                                  out_size) != 0)
        return -1;
    return 0;
}

int helix_gui_list_generations(const char* mnemonic,
                               const char* keychain_path,
                               unsigned char* out_keys, unsigned max,
                               unsigned* count, unsigned* current_gen)
{
    uint8_t mk[HELIX_MASTER_KEY_LEN], fp[KEYCHAIN_FP_LEN];
    char defpath[4096];
    keychain_t kc;
    unsigned i;
    int rc;
    *count = 0;
    *current_gen = 0;
    if (helix_master_key_from_mnemonic(mnemonic, NULL, 0, mk) != 0)
        return -1;
    if (helix_keychain_path(mk, defpath, sizeof defpath) != 0) {
        memset(mk, 0, sizeof mk);
        return -2;
    }
    /* the gui always derives the default fingerprinted path unless a
       cache path was passed explicitly */
    if (keychain_path == NULL)
        keychain_path = defpath;
    rc = keychain_open(&kc, keychain_path, fp);
    if (rc != 0) {
        memset(mk, 0, sizeof mk);
        return -3;
    }
    if (kc.n_entries > max) {
        keychain_close(&kc);
        memset(mk, 0, sizeof mk);
        return -4;
    }
    for (i = 0; i < kc.n_entries; i++) {
        if (keychain_get(&kc, i, out_keys + (size_t)i * 32) != 0) {
            keychain_close(&kc);
            memset(mk, 0, sizeof mk);
            return -3;
        }
    }
    *count = kc.n_entries;
    *current_gen = kc.n_entries ? kc.current_gen : 0;
    keychain_close(&kc);
    memset(mk, 0, sizeof mk);
    return 0;
}

int helix_gui_encrypt(const char* in, const char* out,
                      const char* mnemonic, const char* keychain_path,
                      unsigned generation)
{
    return dna_encrypt_file(in, out, mnemonic, keychain_path, generation);
}

int helix_gui_decrypt(const char* in, const char* out,
                      const char* mnemonic, const char* keychain_path)
{
    return dna_decrypt_file(in, out, mnemonic, keychain_path);
}

int helix_gui_evolve(const char* mnemonic, const char* keychain_path,
                     unsigned* gen_out, unsigned char key[32])
{
    uint32_t g = 0;
    int rc = helix_evolve(mnemonic, keychain_path, &g, key, NULL);
    *gen_out = (unsigned)g;
    return rc;
}

int helix_gui_current_generation(const char* mnemonic,
                                 const char* keychain_path,
                                 unsigned* gen)
{
    uint32_t g = 0;
    int rc = helix_current_generation(mnemonic, keychain_path, &g);
    *gen = (unsigned)g;
    return rc;
}

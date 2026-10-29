#ifndef HELIX_GUI_API_H
#define HELIX_GUI_API_H

#include <stddef.h>
#include <stdint.h>

/* thin stable c abi for the rust gui: every call maps to an existing
   helix api, no logic lives here. error codes mirror dna.h/kdf.h. */

/* gen-seed: fills a space separated mnemonic and its raw entropy */
int helix_gui_gen_seed(unsigned words, char* out_mnemonic,
                       size_t out_size, unsigned char* out_entropy,
                       size_t entropy_len);

/* keychain listing for the key manager. out_keys holds max * 32
   bytes; count receives the number of stored generations.
   computes the master key from the mnemonic (pbkdf2, about 2 s). */
int helix_gui_list_generations(const char* mnemonic,
                               const char* keychain_path,
                               unsigned char* out_keys, unsigned max,
                               unsigned* count, unsigned* current_gen);

int helix_gui_encrypt(const char* in, const char* out,
                      const char* mnemonic, const char* keychain_path,
                      unsigned generation);
int helix_gui_decrypt(const char* in, const char* out,
                      const char* mnemonic, const char* keychain_path);
int helix_gui_encrypt_ex(const char* in, const char* out,
                         const char* mnemonic, const char* keychain_path,
                         unsigned generation, const char* pair_path,
                         const char* pair_pass);
int helix_gui_decrypt_ex(const char* in, const char* out,
                         const char* mnemonic, const char* keychain_path,
                         const char* pair_path, const char* pair_pass);
int helix_gui_pair_create(const char* path, const char* pass,
                          int argon2id);
int helix_gui_pair_export(const char* path, const char* pass,
                          const char* out_path);
int helix_gui_evolve(const char* mnemonic, const char* keychain_path,
                     unsigned* gen_out, unsigned char key[32]);
int helix_gui_current_generation(const char* mnemonic,
                                 const char* keychain_path,
                                 unsigned* gen);

#endif

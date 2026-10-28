#ifndef HELIX_BIP39_H
#define HELIX_BIP39_H

#include <stddef.h>
#include <stdint.h>

#define BIP39_ENTROPY_MIN 16
#define BIP39_ENTROPY_MAX 32
#define BIP39_MNEMONIC_MAX 256   /* 24 words * 12 chars, nul included */
#define BIP39_SEED_LEN    64

/* entropy (16..32 bytes, multiple of 4) -> space separated mnemonic.
   out needs at most BIP39_MNEMONIC_MAX bytes. 0 ok, -1 bad input. */
int bip39_entropy_to_mnemonic(const uint8_t* entropy, size_t len,
                              char* out, size_t out_size);

/* strict single space separators, checksum verified.
   entropy out holds 16/20/24/28/32 bytes for 12/15/18/21/24 words.
   0 ok, -1 bad word count, word or checksum. */
int bip39_mnemonic_to_entropy(const char* mnemonic,
                              uint8_t* out, size_t out_max, size_t* out_len);

/* bip39 seed: pbkdf2-hmac-sha512(mnemonic, "mnemonic" + passphrase,
   2048 iterations). english wordlist is pure ascii so nfkd is the
   identity; non ascii passphrases are not normalized. 0 ok. */
int bip39_seed(const char* mnemonic, const char* passphrase,
               uint8_t out[BIP39_SEED_LEN]);

/* wordlist index of a lowercase word, -1 unknown */
int bip39_word_index(const char* word);

#endif

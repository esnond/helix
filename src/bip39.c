/* bip-0039 mnemonic codec for the english wordlist.
   words are pure ascii (a-z), so nfkd normalization is the identity
   and the bit stream arithmetic is the whole spec. */
#include <string.h>

#include "bip39.h"
#include "sha256.h"
#include "pbkdf2.h"
#include "wordlist.h"

#define SALT_PREFIX "mnemonic"
#define SALT_MAX    (sizeof SALT_PREFIX - 1 + 255)

int bip39_word_index(const char* word)
{
    int lo = 0, hi = BIP39_WORDLIST_SIZE - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        int cmp = strcmp(word, BIP39_WORDS[mid]);
        if (cmp == 0)
            return mid;
        if (cmp < 0)
            hi = mid - 1;
        else
            lo = mid + 1;
    }
    return -1;
}

int bip39_entropy_to_mnemonic(const uint8_t* entropy, size_t len,
                              char* out, size_t out_size)
{
    uint8_t digest[SHA256_DIGEST];
    size_t used = 0;
    int cs_bits, total, words, w, b;
    if (len < BIP39_ENTROPY_MIN || len > BIP39_ENTROPY_MAX || len % 4 != 0)
        return -1;
    cs_bits = (int)len / 4;
    total = (int)len * 8 + cs_bits;
    words = total / 11;
    /* loose precheck: 8 char word plus separator; the per word guard
       below catches the real worst case */
    if (out_size < (size_t)words * 9 + 1)
        return -1;
    sha256(entropy, len, digest);
    for (w = 0; w < words; w++) {
        int idx = 0;
        const char* word;
        size_t n;
        for (b = 0; b < 11; b++) {
            int bitpos = w * 11 + b;
            int bit;
            if (bitpos < (int)len * 8)
                bit = (entropy[bitpos / 8] >> (7 - bitpos % 8)) & 1;
            else
                bit = (digest[0] >> (7 - (bitpos - (int)len * 8))) & 1;
            idx = (idx << 1) | bit;
        }
        word = BIP39_WORDS[idx];
        n = strlen(word);
        if (used + n + (w > 0 ? 1 : 0) + 1 > out_size)
            return -1;
        if (w > 0)
            out[used++] = ' ';
        memcpy(out + used, word, n + 1);
        used += n;
    }
    return 0;
}

/* reads one word into tok, advances *pos past a single separator.
   -1 on empty token, doubled or trailing space, or overlong word. */
static int next_word(const char* mnemonic, size_t* pos,
                     char* tok, size_t tok_size)
{
    size_t start = *pos, n;
    while (mnemonic[*pos] != '\0' && mnemonic[*pos] != ' ')
        (*pos)++;
    n = *pos - start;
    if (n == 0 || n >= tok_size)
        return -1;
    memcpy(tok, mnemonic + start, n);
    tok[n] = '\0';
    if (mnemonic[*pos] == ' ') {
        (*pos)++;
        if (mnemonic[*pos] == '\0' || mnemonic[*pos] == ' ')
            return -1;
    }
    return 0;
}

int bip39_mnemonic_to_entropy(const char* mnemonic,
                              uint8_t* out, size_t out_max, size_t* out_len)
{
    uint8_t stream[33];   /* 264 bits max: 256 entropy + 8 checksum */
    uint8_t digest[SHA256_DIGEST];
    char tok[16];
    size_t pos = 0;
    int words = 0, ent_bits, cs_bits, ent_bytes, i;
    memset(stream, 0, sizeof stream);
    while (mnemonic[pos] != '\0') {
        int idx, base, b;
        if (words >= 24)
            return -1;
        if (next_word(mnemonic, &pos, tok, sizeof tok) != 0)
            return -1;
        idx = bip39_word_index(tok);
        if (idx < 0)
            return -1;
        base = words * 11;
        for (b = 0; b < 11; b++)
            if ((idx >> (10 - b)) & 1)
                stream[(base + b) / 8] |= (uint8_t)(0x80u >> ((base + b) % 8));
        words++;
    }
    if (words != 12 && words != 15 && words != 18 && words != 21 && words != 24)
        return -1;
    cs_bits = words / 3;
    ent_bits = words * 11 - cs_bits;
    ent_bytes = ent_bits / 8;
    if (ent_bytes > (int)out_max)
        return -1;
    sha256(stream, (size_t)ent_bytes, digest);
    for (i = 0; i < cs_bits; i++) {
        int bitpos = ent_bits + i;
        int stored = (stream[bitpos / 8] >> (7 - bitpos % 8)) & 1;
        int want = (digest[0] >> (7 - i)) & 1;
        if (stored != want)
            return -1;
    }
    memcpy(out, stream, (size_t)ent_bytes);
    *out_len = (size_t)ent_bytes;
    return 0;
}

int bip39_seed(const char* mnemonic, const char* passphrase,
               uint8_t out[BIP39_SEED_LEN])
{
    char salt[SALT_MAX];
    size_t n = strlen(passphrase);
    if (n > 255)
        return -1;
    memcpy(salt, SALT_PREFIX, sizeof SALT_PREFIX - 1);
    memcpy(salt + sizeof SALT_PREFIX - 1, passphrase, n);
    pbkdf2_sha512((const uint8_t*)mnemonic, strlen(mnemonic),
                  (const uint8_t*)salt, sizeof SALT_PREFIX - 1 + n,
                  2048, out, BIP39_SEED_LEN);
    memset(salt, 0, sizeof salt);
    return 0;
}

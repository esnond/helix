#ifndef HELIX_KEYCHAIN_H
#define HELIX_KEYCHAIN_H

#include <stddef.h>
#include <stdint.h>

#define KEYCHAIN_MAGIC        "KCH1"
#define KEYCHAIN_VERSION      1
#define KEYCHAIN_HEADER_SIZE  32
#define KEYCHAIN_ENTRY_SIZE   44   /* gen u32 + key 32 + timestamp u64 */
#define KEYCHAIN_FP_LEN       8
#define KEYCHAIN_MAX_ENTRIES  1000000   /* ~44 mb sanity bound */

typedef struct {
    uint32_t generation;
    uint8_t  key[32];
    uint64_t timestamp;   /* unix seconds, metadata only, never in derivation */
} keychain_entry_t;

typedef struct {
    int      fd;
    uint32_t current_gen;   /* highest stored generation */
    uint32_t n_entries;
    uint8_t  master_fp[KEYCHAIN_FP_LEN];
    char     path[4096];
} keychain_t;

/* opens or creates the append-only keychain. validates magic, version,
   master fingerprint and entry structure; a torn trailing entry from a
   crashed append is truncated away on open.
   returns 0 ok, -1 io error, -2 bad magic or version, -3 fingerprint
   mismatch, -4 too many entries. */
int keychain_open(keychain_t* kc, const char* path,
                  const uint8_t master_fp[KEYCHAIN_FP_LEN]);

/* appends one generation; entries must be consecutive from current_gen */
int keychain_append(keychain_t* kc, const uint8_t key[32]);

/* retrieves generation g (0..current_gen) */
int keychain_get(keychain_t* kc, uint32_t g, uint8_t key[32]);

void keychain_close(keychain_t* kc);

#endif

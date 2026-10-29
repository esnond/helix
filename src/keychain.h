#ifndef HELIX_KEYCHAIN_H
#define HELIX_KEYCHAIN_H

#include <stddef.h>
#include <stdint.h>

#define KEYCHAIN_MAGIC        "KCH2"
#define KEYCHAIN_MAGIC_LEGACY "KCH1"
#define KEYCHAIN_VERSION      2
#define KEYCHAIN_HEADER_SIZE  44
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
    uint8_t  mk[64];        /* sealed at open, wiped at close */
    int      legacy;        /* opened as kch1: reads work, writes migrate */
    uint8_t* pt;            /* decrypted entry blob (n_entries records) */
    size_t   pt_cap;
    char     path[4096];
} keychain_t;

/* opens or creates the encrypted keychain. the whole entry area is
   aead(chacha20poly1305) under sha256(mk || "helix-kch"); a wrong
   master key is a fingerprint mismatch (-3) or an authentication
   failure (-4). flock locks the file exclusively for the handle's
   lifetime (-6 when another process holds it). a kch1 file opens
   read-only and migrates to kch2 on the first append (or explicitly
   via keychain_migrate); a torn trailing record is truncated away.
   returns 0 ok, -1 io, -2 bad magic/version, -3 fingerprint mismatch,
   -4 authentication failed / too many entries, -6 busy. */
int keychain_open(keychain_t* kc, const char* path,
                  const uint8_t mk[64]);

/* appends one generation; entries must be consecutive from current_gen.
   a legacy kch1 handle migrates itself first. */
int keychain_append(keychain_t* kc, const uint8_t key[32]);

/* converts an open legacy kch1 handle to kch2 in place (crash safe:
   body first, header flip last). 0 ok, -1 io, -2 format. */
int keychain_migrate(keychain_t* kc);

/* retrieves generation g (0..current_gen) */
int keychain_get(keychain_t* kc, uint32_t g, uint8_t key[32]);

void keychain_close(keychain_t* kc);

#endif

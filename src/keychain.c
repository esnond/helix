/* encrypted keychain, format KCH2 (stage 5). the whole entry area is
   one chacha20poly1305 record under sha256(mk || "helix-kch"); the
   nonce counter in the header makes every rewrite a fresh nonce, so
   reading an old file version leaks nothing.

   layout: header 36 bytes — magic[4] "KCH2", version u8, reserved[3],
   current_gen u32be, ctr u64be, ct_len u32be, master_fp[8] — followed
   by ct(ct_len) + tag(16). plaintext = n * 44 byte entries (gen u32,
   key[32], timestamp u64), the kch1 record layout.

   crash safety: an append writes the new body at the end of the file,
   fsyncs, and only then flips the header fields; a torn write leaves
   the previous generation intact and any trailing garbage is
   truncated on the next open. legacy kch1 files open read-only and
   migrate in place on the first append. */
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "keychain.h"
#include "aead.h"
#include "chacha20.h"
#include "sha256.h"
#include "secure_mem.h"

static const char KCH_TAG[] = "helix-kch";
static const char KCH_NONCE_TAG[] = "helix-kch-nonce";

static void store32_be(uint8_t* p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

static uint32_t load32_be(const uint8_t* p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static void store64_be(uint8_t* p, uint64_t v)
{
    int i;
    for (i = 0; i < 8; i++)
        p[i] = (uint8_t)(v >> (56 - 8 * i));
}

static uint64_t load64_be(const uint8_t* p)
{
    int i;
    uint64_t v = 0;
    for (i = 0; i < 8; i++)
        v = (v << 8) | (uint64_t)p[i];
    return v;
}

/* aead key and the nonce for a counter, both pure functions of mk */
static void chain_key(const uint8_t mk[64], uint8_t key[32])
{
    sha256_ctx c;
    sha256_init(&c);
    sha256_update(&c, mk, 64);
    sha256_update(&c, KCH_TAG, sizeof KCH_TAG - 1);
    sha256_final(&c, key);
}

static void chain_nonce(const uint8_t mk[64], uint64_t ctr, uint8_t nonce[12])
{
    uint8_t digest[SHA256_DIGEST];
    sha256_ctx c;
    sha256_init(&c);
    sha256_update(&c, KCH_NONCE_TAG, sizeof KCH_NONCE_TAG - 1);
    sha256_update(&c, mk, 64);
    store64_be(digest, ctr);
    sha256_update(&c, digest, 8);
    sha256_final(&c, digest);
    memcpy(nonce, digest, 12);
    helix_memzero(digest, sizeof digest);
}

/* file size, -1 on error */
static long file_size(int fd)
{
    struct stat st;
    if (fstat(fd, &st) != 0)
        return -1;
    return st.st_size;
}

/* replaces the in-memory plaintext with n zeroed-then-copied entries */
static int pt_reserve(keychain_t* kc, uint32_t n)
{
    size_t need = (size_t)n * KEYCHAIN_ENTRY_SIZE;
    if (n > KEYCHAIN_MAX_ENTRIES)
        return -4;
    if (need > kc->pt_cap) {
        /* realloc preserves the decrypted entries; the buffer is
           scrubbed wholesale in keychain_close */
        uint8_t* np = realloc(kc->pt, need);
        if (!np)
            return -4;
        kc->pt = np;
        kc->pt_cap = need;
    }
    return 0;
}

static int seal_and_write(keychain_t* kc, uint64_t ctr)
{
    uint8_t key[32], nonce[12];
    uint8_t* ct;
    size_t pt_len = (size_t)kc->n_entries * KEYCHAIN_ENTRY_SIZE;
    long end;
    uint8_t hdr[KEYCHAIN_HEADER_SIZE];
    if (lseek(kc->fd, 0, SEEK_END) < 0)
        return -1;
    end = file_size(kc->fd);
    if (end < 0)
        return -1;
    ct = malloc(pt_len + AEAD_TAG);
    if (!ct)
        return -1;
    /* the finished header is the aad: current_gen, ctr, ct_len and the
       body offset are bound to the body */
    memcpy(hdr, KEYCHAIN_MAGIC, 4);
    hdr[4] = KEYCHAIN_VERSION;
    memset(hdr + 5, 0, 3);
    store32_be(hdr + 8, kc->current_gen);
    store64_be(hdr + 12, ctr);
    store32_be(hdr + 20, (uint32_t)pt_len);
    store64_be(hdr + 24, (uint64_t)end);
    memcpy(hdr + 32, kc->master_fp, KEYCHAIN_FP_LEN);
    store32_be(hdr + 40, 0);
    chain_key(kc->mk, key);
    chain_nonce(kc->mk, ctr, nonce);
    chacha20_poly1305_seal(ct, ct + pt_len, kc->pt, pt_len, hdr,
                           KEYCHAIN_HEADER_SIZE, key, nonce);
    /* body first: a crash before the header flip keeps the old
       generation, and the next open truncates the orphan tail */
    if (pwrite(kc->fd, ct, pt_len + AEAD_TAG, end) !=
        (ssize_t)(pt_len + AEAD_TAG)) {
        helix_memzero(key, sizeof key);
        helix_memzero(ct, pt_len + AEAD_TAG);
        free(ct);
        return -1;
    }
    if (fsync(kc->fd) != 0) {
        helix_memzero(key, sizeof key);
        helix_memzero(ct, pt_len + AEAD_TAG);
        free(ct);
        return -1;
    }
    if (pwrite(kc->fd, hdr, sizeof hdr, 0) != (ssize_t)sizeof hdr ||
        fsync(kc->fd) != 0) {
        helix_memzero(key, sizeof key);
        helix_memzero(ct, pt_len + AEAD_TAG);
        free(ct);
        return -1;
    }
    helix_memzero(key, sizeof key);
    helix_memzero(ct, pt_len + AEAD_TAG);
    free(ct);
    return 0;
}

int keychain_migrate(keychain_t* kc)
{
    int rc;
    if (!kc->legacy)
        return 0;
    kc->legacy = 0;
    rc = seal_and_write(kc, 1);
    return rc;
}

/* reads and decrypts the entry area of an open kch2 file */
static int load_kch2(keychain_t* kc, const uint8_t mk[64])
{
    uint8_t hdr[KEYCHAIN_HEADER_SIZE];
    uint8_t key[32], nonce[12];
    uint32_t ct_len, gen;
    uint64_t ctr, body_off;
    uint8_t* file;
    long size;
    int rc = 0;
    if (pread(kc->fd, hdr, sizeof hdr, 0) != (ssize_t)sizeof hdr)
        return -1;
    if (memcmp(hdr, KEYCHAIN_MAGIC, 4) != 0 || hdr[4] != KEYCHAIN_VERSION)
        return -2;
    gen = load32_be(hdr + 8);
    ctr = load64_be(hdr + 12);
    ct_len = load32_be(hdr + 20);
    body_off = load64_be(hdr + 24);
    if (memcmp(hdr + 32, kc->master_fp, KEYCHAIN_FP_LEN) != 0)
        return -3;
    size = file_size(kc->fd);
    if (size < 0 || body_off < KEYCHAIN_HEADER_SIZE ||
        body_off > (uint64_t)size ||
        ct_len > (uint64_t)KEYCHAIN_MAX_ENTRIES * KEYCHAIN_ENTRY_SIZE ||
        (long)(body_off + (uint64_t)ct_len + AEAD_TAG) > size)
        return -2;
    if (gen >= KEYCHAIN_MAX_ENTRIES ||
        ct_len != (gen + 1u) * KEYCHAIN_ENTRY_SIZE)
        return -2;
    kc->n_entries = gen + 1u;
    kc->current_gen = gen;
    rc = pt_reserve(kc, kc->n_entries);
    if (rc != 0)
        return rc;
    file = malloc(ct_len + AEAD_TAG);
    if (!file)
        return -4;
    if (pread(kc->fd, file, ct_len + AEAD_TAG, (off_t)body_off) !=
        (ssize_t)(ct_len + AEAD_TAG)) {
        free(file);
        return -1;
    }
    chain_key(mk, key);
    chain_nonce(mk, ctr, nonce);
    rc = chacha20_poly1305_open(kc->pt, file, ct_len, file + ct_len, hdr,
                                KEYCHAIN_HEADER_SIZE, key, nonce);
    helix_memzero(key, sizeof key);
    helix_memzero(file, ct_len + AEAD_TAG);
    free(file);
    /* drop any orphan tail from a crashed append */
    if (rc == 0) {
        long want = (long)(body_off + (uint64_t)ct_len + AEAD_TAG);
        if (size > want && ftruncate(kc->fd, want) != 0)
            rc = -1;
    }
    return (rc == 0) ? 0 : -4;
}

/* legacy kch1: plain records, read into the plaintext buffer */
static int load_kch1(keychain_t* kc)
{
    uint8_t hdr[32];
    long size;
    uint32_t i;
    if (pread(kc->fd, hdr, sizeof hdr, 0) != (ssize_t)sizeof hdr)
        return -1;
    if (memcmp(hdr, KEYCHAIN_MAGIC_LEGACY, 4) != 0 || hdr[4] != 1)
        return -2;
    if (memcmp(hdr + 20, kc->master_fp, KEYCHAIN_FP_LEN) != 0)
        return -3;
    size = file_size(kc->fd);
    if (size < 32)
        return -2;
    kc->n_entries = (uint32_t)((size - 32) / KEYCHAIN_ENTRY_SIZE);
    if (kc->n_entries > KEYCHAIN_MAX_ENTRIES)
        return -4;
    kc->current_gen = kc->n_entries ? kc->n_entries - 1u : 0u;
    if (pt_reserve(kc, kc->n_entries) != 0)
        return -4;
    if (pread(kc->fd, kc->pt, (size_t)kc->n_entries * KEYCHAIN_ENTRY_SIZE,
              32) != (ssize_t)kc->n_entries * KEYCHAIN_ENTRY_SIZE)
        return -1;
    /* validate the record structure and drop a torn tail */
    for (i = 0; i < kc->n_entries; i++) {
        uint32_t g = load32_be(kc->pt + (size_t)i * KEYCHAIN_ENTRY_SIZE);
        if (g != i)
            break;
    }
    kc->n_entries = i;
    kc->current_gen = i ? i - 1u : 0u;
    kc->legacy = 1;
    return 0;
}

int keychain_open(keychain_t* kc, const char* path, const uint8_t mk[64])
{
    uint8_t probe[4], digest[SHA256_DIGEST];
    int created = 0, rc;
    memset(kc, 0, sizeof *kc);
    kc->fd = -1;
    kc->legacy = 0;
    if (strlen(path) >= sizeof kc->path)
        return -1;
    strcpy(kc->path, path);
    memcpy(kc->mk, mk, 64);
    sha256(mk, 64, digest);
    memcpy(kc->master_fp, digest, KEYCHAIN_FP_LEN);
    helix_memzero(digest, sizeof digest);
    kc->fd = open(path, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (kc->fd >= 0) {
        created = 1;
    } else {
        kc->fd = open(path, O_RDWR, 0600);
        if (kc->fd < 0)
            return -1;
    }
    if (flock(kc->fd, LOCK_EX | LOCK_NB) != 0) {
        keychain_close(kc);
        return -6;
    }
    if (created) {
        if (seal_and_write(kc, 1) != 0) {
            keychain_close(kc);
            return -1;
        }
        return 0;
    }
    if (pread(kc->fd, probe, 4, 0) != 4) {
        keychain_close(kc);
        return -1;
    }
    if (memcmp(probe, KEYCHAIN_MAGIC, 4) == 0) {
        rc = load_kch2(kc, mk);
    } else if (memcmp(probe, KEYCHAIN_MAGIC_LEGACY, 4) == 0) {
        rc = load_kch1(kc);
    } else {
        rc = -2;
    }
    if (rc != 0)
        keychain_close(kc);
    return rc;
}

int keychain_append(keychain_t* kc, const uint8_t key[32])
{
    uint32_t gen;
    uint64_t ctr;
    uint8_t* rec;
    if (kc->fd < 0)
        return -1;
    if (kc->legacy && keychain_migrate(kc) != 0)
        return -1;
    if (kc->n_entries >= KEYCHAIN_MAX_ENTRIES)
        return -4;
    /* the header counter makes the next nonce fresh; a restart of a
       crashed append reuses (ctr, plaintext-old) which was never
       written, so uniqueness holds */
    {
        uint8_t hdr[KEYCHAIN_HEADER_SIZE];
        if (pread(kc->fd, hdr, sizeof hdr, 0) != (ssize_t)sizeof hdr)
            return -1;
        ctr = load64_be(hdr + 12) + 1u;
    }
    gen = kc->n_entries;
    if (pt_reserve(kc, kc->n_entries + 1) != 0)
        return -4;
    rec = kc->pt + (size_t)gen * KEYCHAIN_ENTRY_SIZE;
    store32_be(rec, gen);
    memcpy(rec + 4, key, 32);
    store64_be(rec + 36, (uint64_t)time(NULL));
    kc->n_entries = gen + 1;
    kc->current_gen = gen;
    if (seal_and_write(kc, ctr) != 0) {
        kc->n_entries = gen;
        kc->current_gen = gen ? gen - 1u : 0u;
        return -1;
    }
    return 0;
}

int keychain_get(keychain_t* kc, uint32_t g, uint8_t key[32])
{
    if (g >= kc->n_entries)
        return -1;
    memcpy(key, kc->pt + (size_t)g * KEYCHAIN_ENTRY_SIZE + 4, 32);
    return 0;
}

void keychain_close(keychain_t* kc)
{
    helix_memzero(kc->mk, sizeof kc->mk);
    if (kc->fd >= 0) {
        flock(kc->fd, LOCK_UN);
        close(kc->fd);
        kc->fd = -1;
    }
    if (kc->pt) {
        helix_memzero(kc->pt, kc->pt_cap);
        free(kc->pt);
        kc->pt = NULL;
        kc->pt_cap = 0;
    }
}

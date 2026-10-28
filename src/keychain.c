/* append only keychain, format KCH1.
   header (32 bytes): magic[4] version u8 reserved[3] current_gen u32
   reserved[9] master_fp[8] reserved[4], all integers big endian.
   entries (44 bytes each): generation u32, key[32], timestamp u64.
   timestamps are audit metadata and never enter key derivation. */
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "keychain.h"

static void store32_be(uint8_t* p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

static void store64_be(uint8_t* p, uint64_t v)
{
    int i;
    for (i = 0; i < 8; i++)
        p[i] = (uint8_t)(v >> (56 - 8 * i));
}

/* scans the entry area, drops a torn trailing record, returns entry
   count or -1 on io error */
static int scan_entries(int fd)
{
    uint8_t rec[KEYCHAIN_ENTRY_SIZE];
    uint64_t off = 0;
    int n = 0;
    for (;;) {
        size_t got = 0;
        while (got < sizeof rec) {
            ssize_t r = pread(fd, rec + got, sizeof rec - got,
                              (off_t)(32 + off + got));
            if (r < 0) {
                if (errno == EINTR)
                    continue;
                return -1;
            }
            if (r == 0)
                break;
            got += (size_t)r;
        }
        if (got == 0)
            break;      /* clean eof at a record boundary */
        if (got < sizeof rec)
            break;      /* torn tail from a crashed append */
        n++;
        off += sizeof rec;
        if (n > KEYCHAIN_MAX_ENTRIES)
            return -2;
    }
    if (lseek(fd, 0, SEEK_END) != (off_t)(32 + off) &&
        ftruncate(fd, (off_t)(32 + off)) != 0)
        return -1;
    return n;
}

int keychain_open(keychain_t* kc, const char* path,
                  const uint8_t master_fp[KEYCHAIN_FP_LEN])
{
    uint8_t hdr[KEYCHAIN_HEADER_SIZE];
    int created = 0, n;
    memset(kc, 0, sizeof *kc);
    kc->fd = -1;
    if (strlen(path) >= sizeof kc->path)
        return -1;
    strcpy(kc->path, path);
    kc->fd = open(path, O_RDWR | O_CREAT | O_APPEND, 0600);
    if (kc->fd < 0)
        return -1;
    if (pread(kc->fd, hdr, sizeof hdr, 0) != (ssize_t)sizeof hdr) {
        if (lseek(kc->fd, 0, SEEK_END) == 0) {
            created = 1;
        } else {
            keychain_close(kc);
            return -1;
        }
    }
    if (created) {
        memset(hdr, 0, sizeof hdr);
        memcpy(hdr, KEYCHAIN_MAGIC, 4);
        hdr[4] = KEYCHAIN_VERSION;
        store32_be(hdr + 8, 0);
        memcpy(hdr + 20, master_fp, KEYCHAIN_FP_LEN);
        if (pwrite(kc->fd, hdr, sizeof hdr, 0) != (ssize_t)sizeof hdr) {
            keychain_close(kc);
            return -1;
        }
    } else {
        if (memcmp(hdr, KEYCHAIN_MAGIC, 4) != 0 || hdr[4] != KEYCHAIN_VERSION) {
            keychain_close(kc);
            return -2;
        }
        if (memcmp(hdr + 20, master_fp, KEYCHAIN_FP_LEN) != 0) {
            keychain_close(kc);
            return -3;
        }
    }
    memcpy(kc->master_fp, master_fp, KEYCHAIN_FP_LEN);
    n = scan_entries(kc->fd);
    if (n < 0) {
        keychain_close(kc);
        return (n == -2) ? -4 : -1;
    }
    kc->n_entries = (uint32_t)n;
    kc->current_gen = (n > 0) ? (uint32_t)(n - 1) : 0;
    return 0;
}

int keychain_append(keychain_t* kc, const uint8_t key[32])
{
    uint8_t rec[KEYCHAIN_ENTRY_SIZE];
    uint32_t gen = kc->n_entries;
    if (kc->n_entries >= KEYCHAIN_MAX_ENTRIES)
        return -4;
    store32_be(rec, gen);
    memcpy(rec + 4, key, 32);
    store64_be(rec + 36, (uint64_t)time(NULL));
    if (write(kc->fd, rec, sizeof rec) != (ssize_t)sizeof rec)
        return -1;
    kc->n_entries = gen + 1;
    kc->current_gen = gen;
    return 0;
}

int keychain_get(keychain_t* kc, uint32_t g, uint8_t key[32])
{
    uint8_t rec[KEYCHAIN_ENTRY_SIZE];
    off_t off;
    if (g >= kc->n_entries)
        return -1;
    off = (off_t)(32 + (uint64_t)g * KEYCHAIN_ENTRY_SIZE) + 4;
    if (pread(kc->fd, rec, 32, off) != 32)
        return -1;
    memcpy(key, rec, 32);
    return 0;
}

void keychain_close(keychain_t* kc)
{
    if (kc->fd >= 0) {
        close(kc->fd);
        kc->fd = -1;
    }
}

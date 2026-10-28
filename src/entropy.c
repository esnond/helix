/* os entropy for seed generation. open/read/close from the allowed
   posix subset (fcntl.h, unistd.h); no libc rand anywhere. */
#include <fcntl.h>
#include <unistd.h>

#include "entropy.h"

int entropy_fill(uint8_t* buf, size_t n)
{
    int fd = open("/dev/urandom", O_RDONLY);
    size_t got = 0;
    if (fd < 0)
        return -1;
    while (got < n) {
        ssize_t r = read(fd, buf + got, n - got);
        if (r <= 0) {
            close(fd);
            return -1;
        }
        got += (size_t)r;
    }
    close(fd);
    return 0;
}

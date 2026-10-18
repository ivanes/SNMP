#include "bsp.h"

#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

extern uint32_t _ebss[];
static uint8_t *s_heap = (uint8_t *)_ebss;

void *_sbrk(ptrdiff_t incr)
{
    uint8_t *prev = s_heap;
    s_heap += incr;
    return prev;
}

int _write(int fd, const void *buf, size_t count)
{
    (void)fd;
    bsp_uart_write((const uint8_t *)buf, count);
    return (int)count;
}

int _read(int fd, void *buf, size_t count)
{
    (void)fd;
    (void)buf;
    (void)count;
    errno = ENOSYS;
    return -1;
}

int _close(int fd)
{
    (void)fd;
    errno = EBADF;
    return -1;
}

int _fstat(int fd, struct stat *st)
{
    (void)fd;
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int fd)
{
    (void)fd;
    return 1;
}

off_t _lseek(int fd, off_t offset, int whence)
{
    (void)fd;
    (void)offset;
    (void)whence;
    return 0;
}

void _exit(int status)
{
    (void)status;
    for (;;) {
    }
}

int _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;
    errno = EINVAL;
    return -1;
}

int _getpid(void)
{
    return 1;
}

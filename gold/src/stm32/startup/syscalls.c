/**
 * @file syscalls.c
 * @brief Minimal system call stubs for bare-metal STM32
 *
 * These stubs provide minimal implementations of POSIX functions
 * required by newlib and libstdc++ for bare-metal operation.
 */

#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

/* Disable semihosting */
void initialise_monitor_handles(void) {}

/* Required by libstdc++ for std::this_thread::sleep_for */
/* Must match POSIX signatures exactly */
__attribute__((used)) unsigned int sleep(unsigned int seconds) {
    /* Simple busy-wait delay (not accurate, just prevents link error) */
    volatile unsigned int count = seconds * 1000000;
    while (count--) {
        __asm__ volatile("nop");
    }
    return 0;
}

__attribute__((used)) int usleep(useconds_t usec) {
    /* Simple busy-wait delay (not accurate, just prevents link error) */
    volatile unsigned int count = usec;
    while (count--) {
        __asm__ volatile("nop");
    }
    return 0;
}

/* Minimal _sbrk for heap allocation */
extern char _end;  /* Defined by linker script */
static char *heap_end = 0;

void *_sbrk(int incr) {
    extern char _estack;  /* Top of stack from linker */
    char *prev_heap_end;

    if (heap_end == 0) {
        heap_end = &_end;
    }
    prev_heap_end = heap_end;

    /* Check for collision with stack */
    if (heap_end + incr > &_estack) {
        errno = ENOMEM;
        return (void *)-1;
    }

    heap_end += incr;
    return prev_heap_end;
}

/* Minimal stubs to satisfy newlib */
int _close(int fd) {
    (void)fd;
    return -1;
}

int _fstat(int fd, struct stat *st) {
    (void)fd;
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int fd) {
    (void)fd;
    return 1;
}

int _lseek(int fd, int ptr, int dir) {
    (void)fd;
    (void)ptr;
    (void)dir;
    return 0;
}

int _read(int fd, char *ptr, int len) {
    (void)fd;
    (void)ptr;
    (void)len;
    return 0;
}

int _write(int fd, char *ptr, int len) {
    (void)fd;
    (void)ptr;
    /* Could redirect to UART here for printf debugging */
    return len;
}

int _getpid(void) {
    return 1;
}

int _kill(int pid, int sig) {
    (void)pid;
    (void)sig;
    errno = EINVAL;
    return -1;
}

void _exit(int status) {
    (void)status;
    while (1) {
        __asm__ volatile("wfi");
    }
}

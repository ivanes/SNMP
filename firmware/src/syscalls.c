#include "bsp.h" /* вывод write() в UART */

#include <sys/stat.h> /* struct stat */
#include <errno.h> /* коды errno */
#include <unistd.h> /* прототипы POSIX-заглушек */

extern uint32_t _ebss[]; /* конец BSS из линкера — старт кучи newlib */
static uint8_t *s_heap = (uint8_t *)_ebss; /* текущий конец кучи sbrk */

void *_sbrk(ptrdiff_t incr) /* расширить кучу newlib */
{ /* начало _sbrk */
    uint8_t *prev = s_heap; /* старый конец */
    s_heap += incr; /* сдвинуть указатель */
    return prev; /* вернуть прежний break */
} /* конец _sbrk */

int _write(int fd, const void *buf, size_t count) /* write() → UART */
{ /* начало _write */
    (void)fd; /* дескриптор не проверяем */
    bsp_uart_write((const uint8_t *)buf, count); /* отправить байты */
    return (int)count; /* всё записано */
} /* конец _write */

int _read(int fd, void *buf, size_t count) /* чтения с stdin нет */
{ /* начало _read */
    (void)fd; /* не используем */
    (void)buf; /* не используем */
    (void)count; /* не используем */
    errno = ENOSYS; /* функция не реализована */
    return -1; /* ошибка */
} /* конец _read */

int _close(int fd) /* закрывать нечего */
{ /* начало _close */
    (void)fd; /* не используем */
    errno = EBADF; /* плохой дескриптор */
    return -1; /* ошибка */
} /* конец _close */

int _fstat(int fd, struct stat *st) /* сказать, что это символ-устройство */
{ /* начало _fstat */
    (void)fd; /* не используем */
    st->st_mode = S_IFCHR; /* character device */
    return 0; /* успех */
} /* конец _fstat */

int _isatty(int fd) /* printf считает вывод терминалом */
{ /* начало _isatty */
    (void)fd; /* любой fd — «терминал» */
    return 1; /* да */
} /* конец _isatty */

off_t _lseek(int fd, off_t offset, int whence) /* позиционирование не нужно */
{ /* начало _lseek */
    (void)fd; /* не используем */
    (void)offset; /* не используем */
    (void)whence; /* не используем */
    return 0; /* остаёмся в начале */
} /* конец _lseek */

void _exit(int status) /* завершение процесса на МК */
{ /* начало _exit */
    (void)status; /* код некуда вернуть */
    for (;;) { /* остановить выполнение */
    } /* конец стопа */
} /* конец _exit */

int _kill(int pid, int sig) /* сигналов нет */
{ /* начало _kill */
    (void)pid; /* не используем */
    (void)sig; /* не используем */
    errno = EINVAL; /* недопустимый вызов */
    return -1; /* ошибка */
} /* конец _kill */

int _getpid(void) /* один «процесс» */
{ /* начало _getpid */
    return 1; /* фиктивный pid */
} /* конец _getpid */

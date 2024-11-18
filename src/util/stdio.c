#include <drivers/screen.h>
#include <stdarg.h>

typedef long long int ssize_t;

typedef enum printf_state {
    PRINTF_STATE_NORMAL,
    PRINTF_STATE_LENGTH,
    PRINTF_STATE_SPECIFIER,
} printf_state_t;

typedef enum printf_length {
    PRINTF_LENGTH_SHORT_SHORT,
    PRINTF_LENGTH_SHORT,
    PRINTF_LENGTH_DEFAULT,
    PRINTF_LENGTH_LONG,
    PRINTF_LENGTH_LONG_LONG,
} printf_length_t;

static char const l[] = "0123456789abcdef";

static void putint(ssize_t arg, int length, unsigned int base)
{
    if (arg < 0)
        putc('-');

    switch (length) {
    case PRINTF_LENGTH_SHORT_SHORT:
        arg = (char)arg;
        break;
    case PRINTF_LENGTH_SHORT:
        arg = (short int)arg;
        break;
    case PRINTF_LENGTH_DEFAULT:
        arg = (int)arg;
        break;
    case PRINTF_LENGTH_LONG:
        arg = (long int)arg;
        break;
    case PRINTF_LENGTH_LONG_LONG:
        arg = (long long int)arg;
        break;
    }

    char buf[25] = { 0 };
    size_t i = 0;
    do {
        buf[i++] = l[arg % base];
        arg /= base;
    } while (arg > 0);
    while (i > 0)
        putc(buf[--i]);
}
static void putuint(size_t arg, int length, unsigned int base)
{
    switch (length) {
    case PRINTF_LENGTH_SHORT_SHORT:
        arg = (unsigned char)arg;
        break;
    case PRINTF_LENGTH_SHORT:
        arg = (unsigned short int)arg;
        break;
    case PRINTF_LENGTH_DEFAULT:
        arg = (unsigned int)arg;
        break;
    case PRINTF_LENGTH_LONG:
        arg = (unsigned long int)arg;
        break;
    case PRINTF_LENGTH_LONG_LONG:
        arg = (unsigned long long int)arg;
    }

    size_t i = 0;
    char buf[25] = { 0 };
    do {
        buf[i++] = l[arg % base];
        arg /= base;
    } while (arg > 0);
    while (i > 0)
        putc(buf[--i]);
}

void __attribute__((format(printf, 1, 2))) printf(char const* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    printf_state_t state = PRINTF_STATE_NORMAL;
    printf_length_t length = PRINTF_LENGTH_DEFAULT;

    while (*fmt) {
        switch (state) {
        case PRINTF_STATE_NORMAL:
            switch (*fmt) {
            case '%':
                state = PRINTF_STATE_LENGTH;
                break;
            default:
                putc(*fmt);
            }
            break;
        case PRINTF_STATE_LENGTH:
            switch (*fmt) {
            case 'h':
                if (length == PRINTF_LENGTH_DEFAULT) {
                    state = PRINTF_STATE_LENGTH;
                    length = PRINTF_LENGTH_SHORT;
                } else {
                    state = PRINTF_STATE_SPECIFIER;
                    length = PRINTF_LENGTH_SHORT_SHORT;
                }
                break;
            case 'l':
                state = PRINTF_STATE_SPECIFIER;
                length = PRINTF_LENGTH_LONG;
                break;
            default:
                state = PRINTF_STATE_SPECIFIER;
                fmt--;
            }
            break;
        case PRINTF_STATE_SPECIFIER:
            switch (*fmt) {
            case 's':
                puts(va_arg(ap, char*));
                break;
            case 'c':
                putc(va_arg(ap, int));
                break;
            case 'd':
            case 'i':
                putint(va_arg(ap, ssize_t), length, 10);
                break;
            case 'o':
                putuint(va_arg(ap, size_t), length, 8);
                break;
            case 'p':
                putc('0');
                putc('x');
                putuint(va_arg(ap, size_t), PRINTF_LENGTH_LONG_LONG, 16);
                break;
            case 'u':
                putuint(va_arg(ap, size_t), length, 10);
                break;
            case 'x':
                putuint(va_arg(ap, size_t), length, 16);
                break;
            case '%':
                putc('%');
                break;
            default:
                putc('%');
                putc(*fmt);
            }
            state = PRINTF_STATE_NORMAL;
            length = PRINTF_LENGTH_DEFAULT;
            break;
        }
        fmt++;
    }
    va_end(ap);
}

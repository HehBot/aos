#include <drivers/screen.h>
#include <stdarg.h>

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
} printf_length_t;

static char const l[] = "0123456789ABCDEF";
static char buf[11];

static void putint(long int arg, int length, unsigned int base)
{
    long int z;
    switch (length) {
    case PRINTF_LENGTH_SHORT_SHORT:
        z = (char)arg;
        break;
    case PRINTF_LENGTH_SHORT:
        z = (short int)arg;
        break;
    case PRINTF_LENGTH_DEFAULT:
        z = (int)arg;
        break;
    case PRINTF_LENGTH_LONG:
        z = arg;
    }

    if (z < 0) {
        putc('-');
        z = -z;
    }

    size_t i = 0;
    do {
        buf[i++] = l[z % base];
        z /= base;
    } while (z > 0);
    while (i > 0)
        putc(buf[--i]);
}
static void putuint(unsigned long int arg, int length, unsigned int base)
{
    unsigned long int z;
    switch (length) {
    case PRINTF_LENGTH_SHORT_SHORT:
        z = (unsigned char)arg;
        break;
    case PRINTF_LENGTH_SHORT:
        z = (unsigned short int)arg;
        break;
    case PRINTF_LENGTH_DEFAULT:
        z = (unsigned int)arg;
        break;
    case PRINTF_LENGTH_LONG:
        z = arg;
    }

    size_t i = 0;
    do {
        buf[i++] = l[z % base];
        z /= base;
    } while (z > 0);
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
                putint(va_arg(ap, long int), length, 10);
                break;
            case 'o':
                putuint(va_arg(ap, unsigned long int), length, 8);
                break;
            case 'u':
                putuint(va_arg(ap, unsigned long int), length, 10);
                break;
            case 'x':
                putuint(va_arg(ap, unsigned long int), length, 16);
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

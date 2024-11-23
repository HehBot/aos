#include <drivers/ega.h>
#include <stdarg.h>
#include <string.h>

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

static size_t get_arg(va_list ap, printf_length_t length, int is_signed)
{
    size_t arg;
    if (is_signed) {
        switch (length) {
        case PRINTF_LENGTH_SHORT_SHORT:
        case PRINTF_LENGTH_SHORT:
        case PRINTF_LENGTH_DEFAULT:
            arg = va_arg(ap, int);
            break;
        case PRINTF_LENGTH_LONG:
            arg = va_arg(ap, long int);
            break;
        case PRINTF_LENGTH_LONG_LONG:
            arg = va_arg(ap, long long int);
            break;
        default:
            __builtin_unreachable();
        }
    } else {
        switch (length) {
        case PRINTF_LENGTH_SHORT_SHORT:
        case PRINTF_LENGTH_SHORT:
        case PRINTF_LENGTH_DEFAULT:
            arg = va_arg(ap, unsigned int);
            break;
        case PRINTF_LENGTH_LONG:
            arg = va_arg(ap, unsigned long int);
            break;
        case PRINTF_LENGTH_LONG_LONG:
            arg = va_arg(ap, unsigned long long int);
            break;
        default:
            __builtin_unreachable();
        }
    }
    return arg;
}

static size_t putuint(size_t arg, unsigned int base, int (*putc)(void*, char), void* put_arg)
{
    int i = 0;
    char buf[25] = { 0 };
    do {
        buf[i++] = l[arg % base];
        arg /= base;
    } while (arg > 0);
    int n = 0;
    while (i > 0)
        n += putc(put_arg, buf[--i]);
    return n;
}
static size_t putint(ssize_t arg, unsigned int base, int (*putc)(void*, char), void* put_arg)
{
    size_t n = 0;
    if (arg < 0) {
        n += putc(put_arg, '-');
        arg = -arg;
    }
    return n + putuint(arg, base, putc, put_arg);
}

int vprintf_put(char const* fmt, va_list ap, int (*putc)(void*, char), int (*puts)(void*, char const*), void* put_arg)
{
    printf_state_t state = PRINTF_STATE_NORMAL;
    printf_length_t length = PRINTF_LENGTH_DEFAULT;

    size_t n = 0;

    while (*fmt) {
        switch (state) {
        case PRINTF_STATE_NORMAL:
            switch (*fmt) {
            case '%':
                state = PRINTF_STATE_LENGTH;
                break;
            default:
                n += putc(put_arg, *fmt);
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
                n += puts(put_arg, va_arg(ap, char const*));
                break;
            case 'c':
                n += putc(put_arg, va_arg(ap, int));
                break;
            case 'd':
            case 'i':
                n += putint(get_arg(ap, length, 1), 10, putc, put_arg);
                break;
            case 'o':
                n += putuint(get_arg(ap, length, 0), 8, putc, put_arg);
                break;
            case 'p':
                n += putc(put_arg, '0');
                n += putc(put_arg, 'x');
                n += putuint(va_arg(ap, uintptr_t), 16, putc, put_arg);
                break;
            case 'u':
                n += putuint(get_arg(ap, length, 0), 10, putc, put_arg);
                break;
            case 'x':
                n += putuint(get_arg(ap, length, 0), 16, putc, put_arg);
                break;
            case '%':
                n += putc(put_arg, '%');
                break;
            default:
                n += putc(put_arg, '%');
                n += putc(put_arg, *fmt);
            }
            state = PRINTF_STATE_NORMAL;
            length = PRINTF_LENGTH_DEFAULT;
            break;
        }
        fmt++;
    }
    return n;
}

static int ega_putc_wrapper(void*, char c)
{
    ega_putc(c);
    return 1;
}
static int ega_puts_wrapper(void*, char const* s)
{
    ega_puts(s);
    return strlen(s);
}
int vprintf(char const* fmt, va_list ap)
{
    return vprintf_put(fmt, ap, &ega_putc_wrapper, &ega_puts_wrapper, NULL);
}
int __attribute__((format(printf, 1, 2))) printf(char const* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = vprintf(fmt, ap);
    va_end(ap);
    return n;
}

struct putc_str_arg {
    char* buf;
    size_t sz;
    size_t pos;
};
int str_putc(void* putc_arg, char c)
{
    struct putc_str_arg* a = putc_arg;
    if (a->pos < a->sz) {
        a->buf[a->pos++] = c;
        return 1;
    }
    return 0;
}
int str_puts(void* putc_arg, char const* s)
{
    struct putc_str_arg* a = putc_arg;
    size_t l = strlen(s);
    size_t rem = a->sz - a->pos;
    if (l > rem)
        l = rem;
    memcpy(&a->buf[a->pos], s, l);
    a->pos += l;
    return l;
}
int vsnprintf(char* buf, size_t sz, char const* fmt, va_list ap)
{
    if (sz == 0)
        return 0;
    else if (sz == 1) {
        buf[0] = 0;
        return 0;
    }
    struct putc_str_arg a = { buf, sz - 1, 0 };
    int n = vprintf_put(fmt, ap, &str_putc, &str_puts, &a);
    buf[a.pos] = '\0';

    return n;
}
int __attribute__((format(printf, 3, 4))) snprintf(char* buf, size_t sz, char const* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sz, fmt, ap);
    va_end(ap);
    return n;
}

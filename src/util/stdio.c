#include <drivers/screen.h>

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

static void putint(int* arg, int length, unsigned int base);
static void putuint(int* arg, int length, unsigned int base);

static char const l[] = "0123456789ABCDEF";
static char buf[11];

void __attribute__((cdecl, format(printf, 1, 2))) printf(char const* fmt, ...)
{
    int* argp = (int*)(&fmt);
    printf_state_t state = PRINTF_STATE_NORMAL;
    printf_length_t length = PRINTF_LENGTH_DEFAULT;

    argp++;
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
                puts((char*)*(argp++));
                break;
            case 'c':
                putc((char)*(argp++));
                break;
            case 'd':
            case 'i':
                putint(argp++, length, 10);
                break;
            case 'o':
                putuint(argp++, length, 8);
                break;
            case 'u':
                putuint(argp++, length, 10);
                break;
            case 'x':
                putuint(argp++, length, 16);
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
}

static void putint(int* arg, int length, unsigned int base)
{
    int i = 0;
    switch (length) {
    case PRINTF_LENGTH_SHORT_SHORT: {
        char z = *(char*)arg;
        if (z < 0) {
            putc('-');
            z = -z;
        }
        do {
            buf[i++] = l[z % base];
            z /= base;
        } while (z > 0);
    } break;
    case PRINTF_LENGTH_SHORT: {
        short int z = *(short int*)arg;
        if (z < 0) {
            putc('-');
            z = -z;
        }
        do {
            buf[i++] = l[z % base];
            z /= base;
        } while (z > 0);
    } break;
    case PRINTF_LENGTH_DEFAULT: {
        int z = *(int*)arg;
        if (z < 0) {
            putc('-');
            z = -z;
        }
        do {
            buf[i++] = l[z % base];
            z /= base;
        } while (z > 0);
    } break;
    case PRINTF_LENGTH_LONG: {
        long int z = *(long int*)arg;
        if (z < 0) {
            putc('-');
            z = -z;
        }
        do {
            buf[i++] = l[z % base];
            z /= base;
        } while (z > 0);
    } break;
    }
    while (i > 0)
        putc(buf[--i]);
}
static void putuint(int* arg, int length, unsigned int base)
{
    int i = 0;
    switch (length) {
    case PRINTF_LENGTH_SHORT_SHORT: {
        unsigned char z = *(unsigned char*)arg;
        do {
            buf[i++] = l[z % base];
            z /= base;
        } while (z > 0);
    } break;
    case PRINTF_LENGTH_SHORT: {
        unsigned short int z = *(unsigned short int*)arg;
        do {
            buf[i++] = l[z % base];
            z /= base;
        } while (z > 0);
    } break;
    case PRINTF_LENGTH_DEFAULT: {
        unsigned int z = *(unsigned int*)arg;
        do {
            buf[i++] = l[z % base];
            z /= base;
        } while (z > 0);
    } break;
    case PRINTF_LENGTH_LONG: {
        unsigned long int z = *(unsigned long int*)arg;
        do {
            buf[i++] = l[z % base];
            z /= base;
        } while (z > 0);
    } break;
    }
    while (i > 0)
        putc(buf[--i]);
}

#include <cpu/interrupt.h>
#include <cpu/port.h>
#include <stdbool.h>
#include <stdio.h>

// static bool lshift_pressed = false;
// static bool rshift_pressed = false;
// static bool lctrl_pressed = false;
// static bool rctrl_pressed = false;
//
// static bool capslock_on = false;
// static bool numlock_on = false;

static char letter(uint8_t scancode)
{
    switch (scancode) {
    case 0x0:
        printf("ERROR ");
        break;
    case 0x1:
        printf("ESC ");
        break;
    case 0x2:
        return '1';
    case 0x3:
        return '2';
    case 0x4:
        return '3';
    case 0x5:
        return '4';
    case 0x6:
        return '5';
    case 0x7:
        return '6';
    case 0x8:
        return '7';
    case 0x9:
        return '8';
    case 0x0A:
        return '9';
    case 0x0B:
        return '0';
    case 0x0C:
        return '-';
    case 0x0D:
        return '+';
    case 0x0E:
        printf("Backspace ");
        break;
    case 0x0F:
        printf("Tab ");
        break;
    case 0x10:
        return 'Q';
    case 0x11:
        return 'W';
    case 0x12:
        return 'E';
    case 0x13:
        return 'R';
    case 0x14:
        return 'T';
    case 0x15:
        return 'Y';
    case 0x16:
        return 'U';
    case 0x17:
        return 'I';
    case 0x18:
        return 'O';
    case 0x19:
        return 'P';
    case 0x1A:
        return '[';
    case 0x1B:
        return ']';
    case 0x1C:
        printf("ENTER ");
        break;
    case 0x1D:
        printf("LCtrl");
        break;
    case 0x1E:
        return 'A';
    case 0x1F:
        return 'S';
    case 0x20:
        return 'D';
    case 0x21:
        return 'F';
    case 0x22:
        return 'G';
    case 0x23:
        return 'H';
    case 0x24:
        return 'J';
    case 0x25:
        return 'K';
    case 0x26:
        return 'L';
    case 0x27:
        return ';';
    case 0x28:
        return '\'';
    case 0x29:
        return '`';
    case 0x2A:
        printf("LShift");
        break;
    case 0x2B:
        return '\\';
    case 0x2C:
        return 'Z';
    case 0x2D:
        return 'X';
    case 0x2E:
        return 'C';
    case 0x2F:
        return 'V';
    case 0x30:
        return 'B';
    case 0x31:
        return 'N';
    case 0x32:
        return 'M';
    case 0x33:
        return ',';
    case 0x34:
        return '.';
    case 0x35:
        return '/';
    case 0x36:
        printf("Rshift ");
        break;
    case 0x37:
        printf("Keypad *");
        break;
    case 0x38:
        printf("LAlt");
        break;
    case 0x39:
        printf("Spc ");
        break;
    case 0x3b:
        printf("F1 ");
        break;
    case 0x3c:
        printf("F2 ");
        break;
    case 0x3d:
        printf("F3 ");
        break;
    case 0x3e:
        printf("F4 ");
        break;
    case 0x3f:
        printf("F5 ");
        break;
    case 0x40:
        printf("F6 ");
        break;
    case 0x41:
        printf("F7 ");
        break;
    case 0x42:
        printf("F8 ");
        break;
    case 0x43:
        printf("F9 ");
        break;
    case 0x44:
        printf("F10 ");
        break;
    case 0x57:
        printf("F11 ");
        break;
    case 0x58:
        printf("F12 ");
        break;
    default:
        /* 'keuyp' event corresponds to the 'keydown' + 0x80
         * it may still be a scancode we haven't implemented yet, or
         * maybe a control/escape sequence */
        if (scancode <= 0x7f) {
            printf("Unknown key down ");
        } else if (scancode <= 0x58 + 0x80) {
            printf("key up ");
            return letter(scancode - 0x80);
        } else
            printf("Unknown key up ");
        break;
    }
    return 0;
}

static void keyboard_callback(cpu_state_t*)
{
    /* The PIC leaves us the scancode in port 0x60 */
    uint8_t sc = port_read_byte(PORT_KEYBOARD_DATA);
    printf("scancode: 0x%hhx, %c", sc, letter(sc));
    printf("\n");
}

void init_keyboard()
{
    register_isr_handler(IRQ1, keyboard_callback);
}

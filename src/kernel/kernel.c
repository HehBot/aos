#include <stdio.h>

void main()
{
    printf("Testing %s %c %d 0o%o %hu 0x%hhx\n", "hello", 'h', -1234567, 012345, 12345678, 0xffffffff);

    asm("int 0x0");
    asm("int 0x1");
    asm("int 0x2");
    asm("int 0x1f");
}

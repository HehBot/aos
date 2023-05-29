unsigned char port_read_byte(unsigned short port)
{
    unsigned char result;
    asm inline("in al, dx"
               : "=a"(result)
               : "d"(port));
    return result;
}
unsigned short port_read_word(unsigned short port)
{
    unsigned short result;
    asm inline("in ax, dx"
               : "=a"(result)
               : "d"(port));
    return result;
}

void port_write_byte(unsigned short port, unsigned char data)
{
    asm inline("out dx, al"
               :
               : "a"(data), "d"(port));
}
void port_write_word(unsigned short port, unsigned short data)
{
    asm inline("out dx, al"
               :
               : "a"(data), "d"(port));
}

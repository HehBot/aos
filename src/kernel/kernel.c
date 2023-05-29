void main()
{
    char* vid_mem = (char*)(0xb8000 + 160);
    char const* str = "In kernel";

    while (*str != 0) {
        *(vid_mem++) = *str++;
        *(vid_mem++) = 0x0f;
    }
}

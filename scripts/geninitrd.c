#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct initrd_header {
    uint8_t magic; // The magic number is there to check for consistency.
    uint8_t name[64];
    uint32_t offset; // Offset in the initrd the file starts.
    uint32_t length; // Length of the file.
} __attribute__((packed));

int main(int argc, char** argv)
{
    uint32_t nheaders = (argc - 1) / 2;
    struct initrd_header headers[64];
    printf("size of header: %lu\n", sizeof(struct initrd_header));
    uint32_t off = sizeof(struct initrd_header) * 64 + sizeof(uint32_t);

    for (int i = 0; i < nheaders; i++) {
        printf("writing file %s->%s at 0x%x\n", argv[i * 2 + 1], argv[i * 2 + 2], off);
        strcpy(headers[i].name, argv[i * 2 + 2]);
        headers[i].offset = off;
        FILE* stream = fopen(argv[i * 2 + 1], "r");
        if (!stream) {
            printf("Error: file not found: %s\n", argv[i * 2 + 1]);
            return 1;
        }
        fseek(stream, 0, SEEK_END);
        headers[i].length = ftell(stream);
        off += headers[i].length;
        fclose(stream);
        headers[i].magic = 0xBF;
    }

    FILE* wstream = fopen("./initrd.img", "w");
    fwrite(&nheaders, sizeof(uint32_t), 1, wstream);
    fwrite(&headers[0], sizeof(struct initrd_header), 64, wstream);

    for (uint32_t i = 0; i < nheaders; i++) {
        FILE* stream = fopen(argv[i * 2 + 1], "r");
        void* buf = malloc(headers[i].length);
        fread(buf, 1, headers[i].length, stream);
        fwrite(buf, 1, headers[i].length, wstream);
        fclose(stream);
        free(buf);
    }

    fclose(wstream);

    return 0;
}

#include <cpu/interrupt.h>
#include <cpu/port.h>
#include <drivers/screen.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// see https://wiki.osdev.org/ATA_PIO_Mode#Addressing_Modes

#define PORT_ATA_DATA 0x1f0
#define PORT_ATA_ERR 0x1f1
#define PORT_ATA_SECCOUNT 0x1f2
#define PORT_ATA_LBA_LO 0x1f3
#define PORT_ATA_LBA_MID 0x1f4
#define PORT_ATA_LBA_HI 0x1f5
#define PORT_ATA_DRIVE_HEAD 0x1f6 // 0xe0 | (drive << 4) | (head)
#define PORT_ATA_STAT_COMM 0x1f7

#define PORT_ATA_CTRL 0x3f6

// byte read from PORT_ATA_STAT_COMM
#define ATA_BSY 0x80 // busy
#define ATA_RDY 0x40 // ready
#define ATA_DF 0x20 // drive fault
#define ATA_ERR 0x01

// byte write to PORT_ATA_STAT_COMM
#define ATA_CMD_READ 0x20
#define ATA_CMD_WRITE 0x30
#define ATA_CMD_RDMUL 0xc4
#define ATA_CMD_WRMUL 0xc5

int havedisk1 = 0;

static int ata_wait(int checkerr)
{
    int r;
    while (((r = port_read_byte(PORT_ATA_STAT_COMM)) & (ATA_BSY | ATA_RDY)) != 0x40)
        ;
    if (checkerr && (r & (ATA_DF | ATA_ERR)) != 0)
        return -1;
    return 0;
}

static void* buf;
static volatile int* flag;

#define SECTOR_SIZE 512

static void ata_callback(cpu_state_t*)
{
    ata_wait(1);
    port_read_word_rep(PORT_ATA_DATA, buf, SECTOR_SIZE / 2);
    *flag = 1;
}

void ata_req(int drive, int sector, void* b, volatile int* f)
{
    buf = b;
    flag = f;
    *f = 0;

    ata_wait(0);
    port_write_byte(PORT_ATA_CTRL, 0); // enable interrupts from ATA
    port_write_byte(PORT_ATA_SECCOUNT, 1);
    port_write_byte(PORT_ATA_LBA_LO, sector & 0xff);
    port_write_byte(PORT_ATA_LBA_MID, (sector >> 8) & 0xff);
    port_write_byte(PORT_ATA_LBA_HI, (sector >> 16) & 0xff);
    port_write_byte(PORT_ATA_DRIVE_HEAD, 0xe0 | ((drive & 1) << 4) | ((sector >> 24) & 0xf));

    port_write_byte(PORT_ATA_STAT_COMM, ATA_CMD_READ);
}

void ata_init(void)
{
    register_isr_handler(IRQ14, &ata_callback);

    ata_wait(0);
    port_write_byte(PORT_ATA_DRIVE_HEAD, 0xe0 | (1 << 4));
    for (int i = 0; i < 1000; ++i) {
        if (port_read_byte(PORT_ATA_STAT_COMM) != 0) {
            printf("[ATA : Disk 1 detected]\n");
            havedisk1 = 1;
            break;
        }
    }
    port_write_byte(PORT_ATA_DRIVE_HEAD, 0xe0 | (0 << 4));
}

#include "block.h"

#include <cpu/interrupt.h>
#include <cpu/port.h>
#include <cpu/x86.h>
#include <liballoc.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

// see https://wiki.osdev.org/ATA_PIO_Mode#Addressing_Modes

#define PORT_ATA_DATA 0x1f0
#define PORT_ATA_ERR 0x1f1
#define PORT_ATA_SECCOUNT 0x1f2
#define PORT_ATA_LBA_LO 0x1f3
#define PORT_ATA_LBA_MID 0x1f4
#define PORT_ATA_LBA_HI 0x1f5
#define PORT_ATA_DRIVE_HEAD 0x1f6
#define PORT_ATA_STAT_COMM 0x1f7

#define LO(s) ((s)&0xff)
#define MID(s) (((s) >> 8) & 0xff)
#define HI(s) (((s) >> 16) & 0xff)
#define DH(d, s) (0xe0 | (((d)&1) << 4) | (((s) >> 24) & 0xf))

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

#define SECTOR_ORDER 9
#define SECTOR_SIZE (1 << SECTOR_ORDER)
#define SECTORS_PER_BLOCK (block_size >> SECTOR_ORDER)

#define SECTOR_OFF (0x100000 / SECTOR_SIZE)

typedef struct sector {
    diskstate_t* state;
    size_t dev;
    size_t sector;
    struct sector* next;
    uint8_t* data;
} sector_t;

static sector_t* ata_queue;

block_t* alloc_block(void)
{
    block_t* b = kmalloc(sizeof(*b));
    b->state = kmalloc(SECTORS_PER_BLOCK * sizeof(b->state[0]));
    memset(b->state, 0, SECTORS_PER_BLOCK * sizeof(b->state[0]));
    b->data = kmalloc(block_size);
    b->next = b->prev = NULL;
    return b;
}
void free_block(block_t* b)
{
    kfree(b->state);
    kfree(b->data);
    kfree(b);
}

static int ata_wait(int checkerr)
{
    int r;
    while (((r = port_read_byte(PORT_ATA_STAT_COMM)) & (ATA_BSY | ATA_RDY)) != 0x40)
        ;
    if (checkerr && (r & (ATA_DF | ATA_ERR)) != 0)
        return -1;
    return 0;
}

static void ata_run(sector_t* s)
{
    if (*s->state == SECTOR_VALID)
        return;
    ata_wait(0);
    port_write_byte(PORT_ATA_CTRL, 0); // enable interrupts from ATA
    port_write_byte(PORT_ATA_SECCOUNT, SECTORS_PER_BLOCK);
    port_write_byte(PORT_ATA_LBA_LO, LO(s->sector));
    port_write_byte(PORT_ATA_LBA_MID, MID(s->sector));
    port_write_byte(PORT_ATA_LBA_HI, HI(s->sector));
    port_write_byte(PORT_ATA_DRIVE_HEAD, DH(s->dev, s->sector));

    if (*s->state == SECTOR_DIRTY) {
        port_write_byte(PORT_ATA_STAT_COMM, ATA_CMD_WRITE);
        port_write_long_rep(PORT_ATA_DATA, s->data, SECTOR_SIZE / 4);
    } else
        port_write_byte(PORT_ATA_STAT_COMM, ATA_CMD_READ);
}

void ata_sync(block_t* b)
{
    sector_t** p = &ata_queue;
    for (; *p != NULL; p = &(*p)->next)
        ;

    int run = (ata_queue == NULL);

    size_t base_sector = b->block * SECTORS_PER_BLOCK + SECTOR_OFF;

    for (size_t i = 0; i < SECTORS_PER_BLOCK; ++i)
        if (b->state[i] == SECTOR_INVALID) {
            sector_t* s = kmalloc(sizeof(*s));
            s->sector = base_sector + i;
            s->dev = b->dev;
            s->state = &b->state[i];
            s->next = NULL;
            s->data = &b->data[i * SECTOR_SIZE];
            *p = s;
            p = &s->next;
        }

    if (run)
        ata_run(ata_queue);

    for (size_t i = 0; i < SECTORS_PER_BLOCK; ++i)
        while (b->state[i] != SECTOR_VALID)
            ;
}

static void ata_callback(cpu_state_t*)
{
    sector_t* s;
    if ((s = ata_queue) == NULL)
        return;

    if (ata_wait(1) < 0)
        return;

    if (*s->state == SECTOR_INVALID)
        port_read_long_rep(PORT_ATA_DATA, s->data, SECTOR_SIZE / 4);
    *s->state = SECTOR_VALID;

    ata_queue = s->next;

    if (ata_queue != NULL)
        ata_run(ata_queue);
}

size_t ata_init(void)
{
    register_irq_handler(T_IRQ14, &ata_callback);

    size_t fsdisk = 0;

    ata_wait(0);
    port_write_byte(PORT_ATA_DRIVE_HEAD, 0xe0 | (1 << 4));
    for (int i = 0; i < 1000; ++i) {
        if (port_read_byte(PORT_ATA_STAT_COMM) != 0) {
            printf("[ATA : Disk 1 detected]\n");
            fsdisk = 1;
            break;
        }
    }
    port_write_byte(PORT_ATA_DRIVE_HEAD, 0xe0 | (0 << 4));
    return fsdisk;
}

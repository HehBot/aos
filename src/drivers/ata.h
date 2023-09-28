#ifndef ATA_H
#define ATA_H

void ata_init(void);
void ata_req(int drive, int sector, void* buf, volatile int* flag);

#endif // ATA_H

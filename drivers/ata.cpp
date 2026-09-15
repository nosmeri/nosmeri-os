#include "ata.h"
#include "io.h"

void ata_wait_ready() {
    while(inb(0x1F7) & 0x80);
}

int ata_read_sector(unsigned int lba, void* buffer) {
    ata_wait_ready();
    outb(0x1F2, 1);
    outb(0x1F3, (unsigned char)lba);
    outb(0x1F4, (unsigned char)(lba>>8));
    outb(0x1F5, (unsigned char)(lba>>16));
    outb(0x1F6, (unsigned char)((lba>>24) | 0xE0));
    outb(0x1F7, 0x20);

    ata_wait_ready();
    while (!(inb(0x1F7) & 0x08));     // DRQ(0x08)가 1이 될 때까지 (준비 완료!)

    for(int i =0; i< 256; i++){
        unsigned short word = inw(0x1F0);
        ((unsigned short*)buffer)[i] = word;
    }
    return 0;
}

int ata_write_sector(unsigned int lba, const void* buffer) {
    ata_wait_ready();
    outb(0x1F2, 1);
    outb(0x1F3, (unsigned char)lba);
    outb(0x1F4, (unsigned char)(lba>>8));
    outb(0x1F5, (unsigned char)(lba>>16));
    outb(0x1F6, (unsigned char)((lba>>24) | 0xE0));
    outb(0x1F7, 0x30);

    ata_wait_ready();
    while (!(inb(0x1F7) & 0x08));     // DRQ(0x08)가 1이 될 때까지 (준비 완료!)

    for(int i =0; i< 256; i++){
        outw(0x1F0, ((unsigned short*)buffer)[i]);
    }

    // 캐시 비우기 (디스크 플래터/플래시에 즉시 영구 기록 강제)
    outb(0x1F7, 0xE7);
    ata_wait_ready();

    return 0;
}


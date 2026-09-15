#pragma once
int ata_read_sector(unsigned int lba, void* buffer);
int ata_write_sector(unsigned int lba, const void* buffer);
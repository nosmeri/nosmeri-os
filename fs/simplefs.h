#pragma once

#define SIMPLEFS_MAGIC 0x50435231 // "PCR1" 매직 넘버
#define INODE_USED     0b01
#define INODE_DIR      0b10

struct Superblock {
    unsigned int magic;
    unsigned int total_sectors;
    unsigned int inode_count;
    unsigned int data_block_start;
} __attribute__((packed)) ;

// 128바이트 -> 한 섹터에 4개씩
struct Inode {
    unsigned int size;
    unsigned int flags;
    unsigned int blocks[30];
} __attribute__((packed));

struct DirEntry {
    unsigned int inode_idx;
    char filename[28];
} __attribute__((packed));

void simplefs_init();
int simplefs_create(unsigned int parent_inode_idx, const char* name);
void simplefs_list(unsigned int dir_inode_idx);
int simplefs_write(unsigned int parent_inode_idx, const char* name, const void* buf, unsigned int size);
int simplefs_read(unsigned int parent_inode_idx, const char* name, void* buf, unsigned int size);
int simplefs_mkdir(unsigned int parent_inode_idx, const char* name);
int simplefs_lookup(unsigned int dir_inode_idx, const char* name);
int simplefs_get_inode(unsigned int inode_idx, Inode* out_inode);
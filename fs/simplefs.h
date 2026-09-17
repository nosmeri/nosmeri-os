#pragma once

#define SIMPLEFS_MAGIC 0x50435231 // "PCR1" 매직 넘버

// inode 플래그
#define INODE_USED     0b01
#define INODE_DIR      0b10

// 디스크 및 섹터 관련
#define SIMPLEFS_SECTOR_SIZE        512 // 섹터 한개의 크기(바이트)
#define SIMPLEFS_TOTAL_SECTORS      32768   // 16MB 디스크
#define SIMPLEFS_SUPERBLOCK_SECTOR  0 // 슈퍼블럭이 위치한 섹터

// Inode 관련
#define SIMPLEFS_INODE_START_SECTOR 1 // Inode가 시작되는 섹터
#define SIMPLEFS_INODE_COUNT        64 // Inode의 개수
#define SIMPLEFS_INODES_PER_SECTOR  (SIMPLEFS_SECTOR_SIZE / sizeof(Inode)) // 512 / 128 = 4
#define SIMPLEFS_INODE_SECTORS      (SIMPLEFS_INODE_COUNT / SIMPLEFS_INODES_PER_SECTOR) // 64 / 4 = 16
#define SIMPLEFS_DATA_START_SECTOR  (SIMPLEFS_INODE_START_SECTOR + SIMPLEFS_INODE_SECTORS) // 17 데이터 영역이 시작되는 섹터
#define SIMPLEFS_DIRECT_BLOCKS      30 // Inode당 직접 블록 개수

// 디렉터리 엔트리 관련
#define SIMPLEFS_MAX_FILENAME       28 // 파일 이름의 최대 길이
#define SIMPLEFS_ENTRIES_PER_BLOCK  (SIMPLEFS_SECTOR_SIZE / sizeof(DirEntry)) // 16 한 섹터에 들어가는 디렉터리 엔트리의 수


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
    unsigned int blocks[SIMPLEFS_DIRECT_BLOCKS];
} __attribute__((packed));

struct DirEntry {
    unsigned int inode_idx;
    char filename[SIMPLEFS_MAX_FILENAME];
} __attribute__((packed));

void simplefs_init();
int simplefs_create(unsigned int parent_inode_idx, const char* name);
int simplefs_get_direntry(unsigned int dir_inode_idx, int index, DirEntry* out_entry);
int simplefs_write(unsigned int parent_inode_idx, const char* name, const void* buf, unsigned int size);
int simplefs_read(unsigned int parent_inode_idx, const char* name, void* buf, unsigned int size);
int simplefs_mkdir(unsigned int parent_inode_idx, const char* name);
int simplefs_lookup(unsigned int dir_inode_idx, const char* name);
int simplefs_get_inode(unsigned int inode_idx, Inode* out_inode);
unsigned int find_free_data_block();
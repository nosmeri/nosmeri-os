#include "simplefs.h"
#include "ata.h"
#include "vga.h"
#include "heap.h"
#include "string.h"

static Superblock sb;

void simplefs_init() {
    char buffer[512];

    // 0번섹터 읽어오기
    ata_read_sector(SIMPLEFS_SUPERBLOCK_SECTOR, &buffer);
    memcpy(&sb, buffer, sizeof(Superblock));

    // 매직넘버 못찾으면 포맷
    if(sb.magic != SIMPLEFS_MAGIC || sb.total_sectors == 0) {
        print_string("[SimpleFS] Formatting Disk...");

        sb.magic = SIMPLEFS_MAGIC;
        sb.inode_count = SIMPLEFS_INODE_COUNT;
        sb.data_block_start = SIMPLEFS_DATA_START_SECTOR;
        sb.total_sectors = SIMPLEFS_TOTAL_SECTORS;

        memset(buffer, 0, 512);
        memcpy(buffer, &sb, sizeof(Superblock));

        ata_write_sector(SIMPLEFS_SUPERBLOCK_SECTOR, buffer);

        // 모든 inode 초기화
        print_string("Clearing Inodes...");
        char zero_buf[512];
        memset(zero_buf, 0, 512);
        for (unsigned int sec = SIMPLEFS_INODE_START_SECTOR; sec < SIMPLEFS_DATA_START_SECTOR; sec++) {
            ata_write_sector(sec, zero_buf);
        }

        print_string("Creating root directory...");
        // 루트 디렉터리 inode 생성
        memset(buffer, 0, 512);
        Inode* inodes = (Inode*)buffer;
        inodes[0].size = 0;
        inodes[0].flags = INODE_USED | INODE_DIR;
        memset(inodes[0].blocks, 0, sizeof(inodes[0].blocks));
        inodes[0].blocks[0] = sb.data_block_start;
        ata_write_sector(SIMPLEFS_INODE_START_SECTOR, buffer);

        // 루트 디렉터리 생성
        DirEntry* root_entries = (DirEntry*)buffer;
        memset(root_entries, 0, 512);
        root_entries[0].inode_idx = 0;
        memcpy(root_entries[0].filename, ".", 2);
        root_entries[1].inode_idx = 0;
        memcpy(root_entries[1].filename, "..", 3);
        ata_write_sector(sb.data_block_start, buffer);

        print_string("Done.\n");
        return;
    }

    print_string("[SimpleFS] Disk mounted successfully.");
}

static int allocate_free_inode(unsigned int flags) {
    // inode 테이블 순회하며 빈 inode 탐색
    int target_sector = -1;
    int target_index = -1;
    int target_inode_idx = -1;
    unsigned char buffer[512];

    for (unsigned int sec = SIMPLEFS_INODE_START_SECTOR; sec < SIMPLEFS_DATA_START_SECTOR; sec++) {
        ata_read_sector(sec, buffer);
        Inode* inodes = (Inode*)buffer;
        for (unsigned int i = 0; i < SIMPLEFS_INODES_PER_SECTOR; i++) {
            if((inodes[i].flags & INODE_USED) == 0 && target_sector == -1) {
                target_sector = sec;
                target_index = i;
                target_inode_idx = (sec - SIMPLEFS_INODE_START_SECTOR) * SIMPLEFS_INODES_PER_SECTOR + i;
                break;
            }
        }
        if(target_sector != -1) break;
    }
    if (target_sector == -1) return -1;

    // 빈 inode 사용
    ata_read_sector(target_sector, buffer);
    Inode* inodes = (Inode*)buffer;
    inodes[target_index].size = 0;
    inodes[target_index].flags = flags;
    memset(inodes[target_index].blocks, 0, sizeof(inodes[target_index].blocks));

    ata_write_sector(target_sector, buffer);

    return target_inode_idx;
}
static int add_dir_entry(unsigned int dir_block_num, unsigned int inode_idx, const char* name) {
    DirEntry parent_direntry[SIMPLEFS_ENTRIES_PER_BLOCK];
    ata_read_sector(dir_block_num, (char*)parent_direntry);
    for(unsigned int i = 0; i < SIMPLEFS_ENTRIES_PER_BLOCK; i++) {
        if(parent_direntry[i].filename[0] == '\0') {
            parent_direntry[i].inode_idx = inode_idx;
            memcpy(parent_direntry[i].filename, name, strlen(name)+1); // 널 문자까지 복사
            ata_write_sector(dir_block_num, (char*)parent_direntry);
            return 0;
        }
    }
    return -1;
}
static int write_inode(unsigned int inode_idx, const Inode* inode) {
    int target_sector = inode_idx / SIMPLEFS_INODES_PER_SECTOR + SIMPLEFS_INODE_START_SECTOR;
    int target_index = inode_idx % SIMPLEFS_INODES_PER_SECTOR;
    
    unsigned char buffer[512];
    ata_read_sector(target_sector, buffer);
    Inode* inodes = (Inode*)buffer;
    inodes[target_index] = *inode;
    ata_write_sector(target_sector, buffer);

    return 0;
}
static int read_inode(unsigned int inode_idx, Inode* inode) {
    int target_sector = inode_idx / SIMPLEFS_INODES_PER_SECTOR + SIMPLEFS_INODE_START_SECTOR;
    int target_index = inode_idx % SIMPLEFS_INODES_PER_SECTOR;
    
    unsigned char buffer[512];
    ata_read_sector(target_sector, buffer);
    *inode = ((Inode*)buffer)[target_index];

    return 0;
}

// 파일 생성
int simplefs_create(unsigned int parent_inode_idx, const char* name) {
    if (strlen(name) >= SIMPLEFS_MAX_FILENAME || strlen(name) == 0) return -1;

    // 부모 디렉터리 inode 위치
    Inode parent_inode;
    read_inode(parent_inode_idx, &parent_inode);

    // 부모 디렉터리 플래그 확인
    if((parent_inode.flags & INODE_DIR) == 0) return -1;

    // 부모 디렉터리 데이터 블록(테이블)
    unsigned int parent_data_block_num = parent_inode.blocks[0];

    // 부모 디렉터리 테이블 순회하며 중복 파일 존재 확인
    if (simplefs_lookup(parent_inode_idx, name) != -1) return -1;

    // inode 테이블 순회하며 빈 inode 탐색
    int target_inode_idx = allocate_free_inode(INODE_USED);
    if (target_inode_idx == -1) return -1;

    // 부모 디렉터리 테이블에 파일 추가
    if(add_dir_entry(parent_data_block_num, target_inode_idx, name) != 0) return -1;

    return 0;
}

// 폴더 만들기
int simplefs_mkdir(unsigned int parent_inode_idx, const char* name) {
    if (strlen(name) >= SIMPLEFS_MAX_FILENAME || strlen(name) == 0) return -1;

    // 부모 디렉터리 inode 위치
    Inode parent_inode;
    read_inode(parent_inode_idx, &parent_inode);

    // 부모 디렉터리 플래그 확인
    if((parent_inode.flags & INODE_DIR) == 0) return -1;

    // 부모 디렉터리 데이터 블록(테이블)
    unsigned int parent_data_block_num = parent_inode.blocks[0];

    // 부모 디렉터리 테이블 순회하며 중복 파일 존재 확인
    if (simplefs_lookup(parent_inode_idx, name) != -1) return -1;
    
    // inode 테이블 순회하며 빈 inode 탐색
    int target_inode_idx = allocate_free_inode(INODE_USED | INODE_DIR);
    if (target_inode_idx == -1) return -1;

    // 디렉터리 블록에 . 와 .. 추가
    unsigned int new_dir_block = find_free_data_block();
    DirEntry dir_entries[SIMPLEFS_ENTRIES_PER_BLOCK];
    memset(dir_entries, 0, 512);
    dir_entries[0].inode_idx = target_inode_idx;
    memcpy(dir_entries[0].filename, ".", 1);
    dir_entries[1].inode_idx = parent_inode_idx;
    memcpy(dir_entries[1].filename, "..", 2);
    ata_write_sector(new_dir_block, dir_entries);

    // 디렉터리 inode 업데이트
    Inode new_dir_inode;
    read_inode(target_inode_idx, &new_dir_inode);
    new_dir_inode.blocks[0] = new_dir_block;
    write_inode(target_inode_idx, &new_dir_inode);

    // 부모 디렉터리 테이블에 파일 추가
    if(add_dir_entry(parent_data_block_num, target_inode_idx, name) != 0) return -1;

    return 0;
}

// dir의 index 번째 파일 가져와서 out_entry에 저장
// index가 범위를 벗어나거나 존재하지 않는 파일이면 -1 반환
int simplefs_get_direntry(unsigned int dir_inode_idx, int index, DirEntry* out_entry) {
    if (index < 0 || (unsigned int)index >= SIMPLEFS_ENTRIES_PER_BLOCK) return -1;
    
    Inode inode;
    read_inode(dir_inode_idx, &inode);

    if ((inode.flags & INODE_DIR) == 0) return -1;

    // dir inode의 첫번째 블록(디렉터리 테이블) 읽기
    unsigned int dir_block = inode.blocks[0];
    unsigned char buffer[512];
    ata_read_sector(dir_block, buffer);
    DirEntry* direntry = (DirEntry*)buffer;

    // 존재하지 않는 파일이면 -1 반환
    if (direntry[index].filename[0] == '\0') return -1;

    memcpy(out_entry, direntry+index, sizeof(DirEntry));

    return 0;
}

// 디렉토리에 파일이 존재하는지 확인
int simplefs_lookup(unsigned int dir_inode_idx, const char* name) {
    Inode inode;
    read_inode(dir_inode_idx, &inode);

    if((inode.flags & INODE_DIR) == 0) return -1;

    unsigned int dir_block = inode.blocks[0];
    unsigned char buffer[512];
    ata_read_sector(dir_block, buffer);
    DirEntry* direntry = (DirEntry*)buffer;

    for(unsigned int i = 0; i < SIMPLEFS_ENTRIES_PER_BLOCK; i++) {
        if(direntry[i].filename[0] != '\0') {
            if(strcmp(direntry[i].filename, name) == 0) {
                return direntry[i].inode_idx;
            }
        }
    }
    return -1;
}

bool is_block_used(unsigned int block_num) {
    unsigned char buffer[512];
    for (unsigned int sec = SIMPLEFS_INODE_START_SECTOR; sec < SIMPLEFS_DATA_START_SECTOR; sec++) {
        ata_read_sector(sec, buffer);
        Inode* inodes = (Inode*) buffer;
        for (unsigned int i=0; i < SIMPLEFS_INODES_PER_SECTOR; i++) {
            if (inodes[i].flags & INODE_USED) {
                for (unsigned int j = 0; j < SIMPLEFS_DIRECT_BLOCKS; j++) {
                    if (inodes[i].blocks[j] == block_num) return true;
                }
            }
        }
    }
    return false;
}

unsigned int find_free_data_block() {
    unsigned int candidate = sb.data_block_start;
    for (;is_block_used(candidate);candidate++);
    return candidate;
}

int simplefs_write(unsigned int parent_inode_idx, const char* name, const void* buf, unsigned int size) {
    int file_inode_idx = simplefs_lookup(parent_inode_idx, name);
    if (file_inode_idx == -1) return -1;

    unsigned int needed_blocks = (size + 511) / 512;

    if (needed_blocks > SIMPLEFS_DIRECT_BLOCKS) return -1;

    Inode file_inode;
    read_inode(file_inode_idx, &file_inode);

    unsigned int bytes_left = size;
    unsigned char buffer[512];
    for (unsigned int i=0; i < needed_blocks; i++) {
        // 블록이 할당되지 않았을 경우 -> 새로운 블록 탐색 및 할당
        if (file_inode.blocks[i] == 0) {
            unsigned int block = find_free_data_block();
            file_inode.blocks[i] = block;
            write_inode(file_inode_idx, &file_inode);
        }
        // 쓸 데이터 크기 결정
        unsigned int chunk = (bytes_left > 512) ? 512 : bytes_left;
        // 데이터 쓰기
        memset(buffer, 0, 512);
        memcpy(buffer, (unsigned char*)buf + i * 512, chunk);
        ata_write_sector(file_inode.blocks[i], buffer);
        bytes_left -= chunk;
    }
    
    file_inode.size = size;
    write_inode(file_inode_idx, &file_inode);
    return 0;
}

int simplefs_read(unsigned int parent_inode_idx, const char* name, void* buf, unsigned int size) {
    int file_inode_idx = simplefs_lookup(parent_inode_idx, name);
    if (file_inode_idx == -1) return -1;
    
    Inode file_inode;
    read_inode(file_inode_idx, &file_inode);
    if (file_inode.flags & INODE_DIR) return -1;

    unsigned int bytes_to_read = (size > file_inode.size) ? file_inode.size : size;

    unsigned int needed_blocks = (bytes_to_read + 511) / 512;

    unsigned int bytes_left = bytes_to_read;
    unsigned char buffer[512];
    for (unsigned int i=0; i < needed_blocks; i++) {
        // 읽을 데이터 크기 결정
        unsigned int chunk = (bytes_left > 512) ? 512 : bytes_left;
        // 데이터 읽기
        ata_read_sector(file_inode.blocks[i], buffer);
        memcpy((unsigned char*)buf + i * 512, buffer, chunk);
        bytes_left -= chunk;
    }
    return bytes_to_read;
}

int simplefs_get_inode(unsigned int inode_idx, Inode* out_inode) {
    if (inode_idx >= SIMPLEFS_INODE_COUNT || !out_inode) return -1;

    int sector = inode_idx / SIMPLEFS_INODES_PER_SECTOR + SIMPLEFS_INODE_START_SECTOR;
    int index = inode_idx % SIMPLEFS_INODES_PER_SECTOR;

    unsigned char buffer[512];
    ata_read_sector(sector, buffer);
    Inode* inodes = (Inode*)buffer;
    *out_inode = inodes[index];
    return 0;
}
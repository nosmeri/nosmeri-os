#pragma once

#define VFS_FILE      0b01
#define VFS_DIRECTORY 0b10

struct vfs_node {
    char name[32];           // 파일 또는 폴더 이름
    unsigned int flags;      // VFS_FILE 또는 VFS_DIRECTORY
    unsigned int size;       // 파일 크기
    unsigned int inode_idx;  // SimpleFS 상의 Inode 번호 (0~63)
    unsigned int parent_idx; // 부모 Inode 번호
};

extern vfs_node vfs_root;
extern vfs_node current_dir;

void vfs_init();
int vfs_lookup(const vfs_node* dir_node, const char* name, vfs_node* out_node);
int vfs_create(const vfs_node* parent, const char* name, vfs_node* out_node = 0);
int vfs_mkdir(const vfs_node* parent, const char* name, vfs_node* out_node = 0);
int vfs_write(vfs_node* file, const void* buf, unsigned int size);
int vfs_read(vfs_node* file, void* buf, unsigned int size);
int vfs_readdir(const vfs_node* dir, int index, vfs_node* out_node);
int vfs_resolve_path(const char* path, vfs_node* out_node);
int vfs_cd(const char* path);
int vfs_get_path(const vfs_node* node, char* out_buf, unsigned int buf_size);
int vfs_getcwd(char* out_buf, unsigned int buf_size);

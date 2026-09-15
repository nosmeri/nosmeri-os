#include "vfs.h"
#include "simplefs.h"
#include "string.h"

vfs_node vfs_root;
vfs_node current_dir;
char current_path[256] = "/";

void vfs_init() {
    simplefs_init();

    // vfs root 설정
    memset(&vfs_root, 0, sizeof(vfs_node));
    memcpy(vfs_root.name, "/", 2);
    vfs_root.flags = VFS_DIRECTORY;
    vfs_root.inode_idx = 0;
    vfs_root.parent_idx = 0;

    Inode root_inode;
    if (simplefs_get_inode(0, &root_inode) == 0) {
        vfs_root.size = root_inode.size;
    }

    current_dir = vfs_root;
    memcpy(current_path, "/", 2);
}

int vfs_lookup(const vfs_node* dir_node, const char* name, vfs_node* out_node) {
    if (!dir_node || !(dir_node->flags & VFS_DIRECTORY) || !out_node) return -1;

    if (strcmp(name, ".") == 0) {
        *out_node = *dir_node;
        return 0;
    }

    int target_inode_idx = simplefs_lookup(dir_node->inode_idx, name);
    if (target_inode_idx < 0) return -1;

    Inode target_inode;
    if (simplefs_get_inode(target_inode_idx, &target_inode) != 0) return -1;

    memset(out_node, 0, sizeof(vfs_node));
    memcpy(out_node->name, name, strlen(name) + 1);
    out_node->inode_idx = target_inode_idx;
    out_node->parent_idx = dir_node->inode_idx;
    out_node->size = target_inode.size;

    if (target_inode.flags & INODE_DIR) {
        out_node->flags = VFS_DIRECTORY;
    } else {
        out_node->flags = VFS_FILE;
    }

    return 0;
}

int vfs_create(const vfs_node* parent, const char* name, vfs_node* out_node) {
    if (!parent) return -1;

    // 이미 존재하는 파일인지 확인 (스택 변수 사용으로 누수 없음)
    vfs_node dummy;
    if (vfs_lookup(parent, name, &dummy) == 0) return -1;

    // 파일 생성
    int result = simplefs_create(parent->inode_idx, name);
    if (result < 0) return -1;

    // 요청 시 VFS 노드 정보 채움
    if (out_node) {
        vfs_lookup(parent, name, out_node);
    }
    return 0;
}

int vfs_mkdir(const vfs_node* parent, const char* name, vfs_node* out_node) {
    if (!parent) return -1;

    // 이미 존재하는 디렉터리인지 확인
    vfs_node dummy;
    if (vfs_lookup(parent, name, &dummy) == 0) return -1;

    // 디렉토리 생성
    int result = simplefs_mkdir(parent->inode_idx, name);
    if (result < 0) return -1;

    // 요청 시 VFS 노드 정보 채움
    if (out_node) {
        vfs_lookup(parent, name, out_node);
    }
    return 0;
}

int vfs_write(vfs_node* file, const void* buf, unsigned int size) {
    if (!file || (file->flags & VFS_DIRECTORY)) return -1; // 디렉토리 보호
    int result = simplefs_write(file->parent_idx, file->name, buf, size);
    if (result < 0) return -1;
    file->size = size;
    return result;
}

int vfs_read(vfs_node* file, void* buf, unsigned int size) {
    if (!file || (file->flags & VFS_DIRECTORY)) return -1; // 디렉토리 보호
    int result = simplefs_read(file->parent_idx, file->name, buf, size);
    return result;
}

void vfs_list(const vfs_node* dir) {
    if (!dir) return;
    simplefs_list(dir->inode_idx);
}

static int resolve_step(vfs_node* cur, const char* token, char* out_canonical_path) {
    vfs_node next;
    if (vfs_lookup(cur, token, &next) != 0) return -1;
    *cur = next;

    if (out_canonical_path) {
        if (strcmp(token, ".") == 0) {
            // 현재 디렉터리는 유지
        } else if (strcmp(token, "..") == 0) {
            int len = strlen(out_canonical_path);
            while (len > 0 && out_canonical_path[len - 1] != '/') len--;
            if (len > 0) len--;
            out_canonical_path[len] = '\0';
        } else {
            int len = strlen(out_canonical_path);
            out_canonical_path[len++] = '/';
            memcpy(out_canonical_path + len, token, strlen(token) + 1);
        }
    }
    return 0;
}

int vfs_resolve_path(const char* path, vfs_node* out_node, char* out_canonical_path) {
    if (!out_node) return -1;

    if (!path || path[0] == '\0') {
        *out_node = current_dir;
        if (out_canonical_path) {
            memcpy(out_canonical_path, current_path, strlen(current_path) + 1);
        }
        return 0;
    }

    vfs_node cur;
    int i = 0;
    
    if (path[0] == '/') {
        cur = vfs_root;
        if (out_canonical_path) {
            out_canonical_path[0] = '\0';
        }
        while (path[i] == '/') i++;
    } else {
        cur = current_dir;
        if (out_canonical_path) {
            if (strcmp(current_path, "/") == 0) {
                out_canonical_path[0] = '\0';
            } else {
                memcpy(out_canonical_path, current_path, strlen(current_path) + 1);
            }
        }
    }
    
    char token[256];
    int n = 0;
    while (path[i] != '\0') {
        if (path[i] == '/') {
            if (n > 0) {
                token[n] = '\0';
                if (resolve_step(&cur, token, out_canonical_path) != 0) return -1;
                n = 0;
            }
        } else {
            token[n++] = path[i];
        }
        i++;
    }

    if (n > 0) {
        token[n] = '\0';
        if (resolve_step(&cur, token, out_canonical_path) != 0) return -1;
    }

    if (out_canonical_path && out_canonical_path[0] == '\0') {
        memcpy(out_canonical_path, "/", 2);
    }

    *out_node = cur;
    return 0;
}

int vfs_cd(const char* path) {
    vfs_node target;
    char new_path[256];
    if (vfs_resolve_path(path, &target, new_path) != 0) return -1;
    if (!(target.flags & VFS_DIRECTORY)) return -1;

    current_dir = target;
    memcpy(current_path, new_path, strlen(new_path) + 1);
    return 0;
}


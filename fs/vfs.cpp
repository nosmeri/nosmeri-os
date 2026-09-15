#include "vfs.h"
#include "simplefs.h"
#include "string.h"

vfs_node vfs_root;
vfs_node current_dir;
char current_path[256] = "/";

// vfs(virtual file system) 설정
void vfs_init() {
    simplefs_init();

    // vfs root 노드 설정
    memset(&vfs_root, 0, sizeof(vfs_node));
    memcpy(vfs_root.name, "/", 2);
    vfs_root.flags = VFS_DIRECTORY;
    vfs_root.inode_idx = 0;
    vfs_root.parent_idx = 0;

    // 루트 폴더 크기 가져오기
    Inode root_inode;
    if (simplefs_get_inode(0, &root_inode) == 0) {
        vfs_root.size = root_inode.size;
    }

    // 현재 폴더를 루트폴더로
    current_dir = vfs_root;
    memcpy(current_path, "/", 2);
}

// dir_node 폴더에 name 이름의 파일이나 폴더가 존재하면 out_node 에 저장
int vfs_lookup(const vfs_node* dir_node, const char* name, vfs_node* out_node) {
    if (!dir_node || !(dir_node->flags & VFS_DIRECTORY) || !out_node) return -1;

    // 현재 디렉터리
    if (strcmp(name, ".") == 0) {
        *out_node = *dir_node;
        return 0;
    }

    // 부모 디렉터리 inode에서 name 이름으로 파일이나 폴더의 inode idx 찾기
    int target_inode_idx = simplefs_lookup(dir_node->inode_idx, name);
    if (target_inode_idx < 0) return -1;

    // 디렉터리 안에 target inode 찾았다면 inode 정보 가져오기
    Inode target_inode;
    if (simplefs_get_inode(target_inode_idx, &target_inode) != 0) return -1;

    // out_node에 이름, inode idx, parent idx, size 저장
    memset(out_node, 0, sizeof(vfs_node));
    memcpy(out_node->name, name, strlen(name) + 1);
    out_node->inode_idx = target_inode_idx;
    out_node->parent_idx = dir_node->inode_idx;
    out_node->size = target_inode.size;

    // 디렉터리인지 파일인지 확인하고 out_node의 flag 설정
    if (target_inode.flags & INODE_DIR) {
        out_node->flags = VFS_DIRECTORY;
    } else {
        out_node->flags = VFS_FILE;
    }

    return 0;
}

// parent 폴더에 name 이름으로 파일 생성해서 out_node에 저장(저장은 선택)
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

// parent 폴더에 name 이름으로 폴더 생성해서 out_node에 저장(저장은 선택)
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

// file에 buf를 쓰기
int vfs_write(vfs_node* file, const void* buf, unsigned int size) {
    if (!file || (file->flags & VFS_DIRECTORY)) return -1; // 디렉토리 보호
    int result = simplefs_write(file->parent_idx, file->name, buf, size);
    if (result < 0) return -1;
    file->size = size;
    return result;
}

// file 내용을 buf에 저장
int vfs_read(vfs_node* file, void* buf, unsigned int size) {
    if (!file || (file->flags & VFS_DIRECTORY)) return -1; // 디렉토리 보호
    int result = simplefs_read(file->parent_idx, file->name, buf, size);
    return result;
}

// dir 폴더의 index 번째 파일 정보를 out_node에 저장
int vfs_readdir(const vfs_node* dir, int index, vfs_node* out_node) {
    if (!dir || !(dir->flags & VFS_DIRECTORY) || !out_node) return -1;
    // dir의 index 번째 파일 이름 가져오기    
    DirEntry dir_entry;
    if (simplefs_get_direntry(dir->inode_idx, index, &dir_entry) == -1) {
        return -1;
    }

    Inode inode;
    simplefs_get_inode(dir_entry.inode_idx, &inode);
    memcpy(out_node->name, dir_entry.filename, strlen(dir_entry.filename) + 1);
    out_node->size=inode.size;
    out_node->inode_idx=dir_entry.inode_idx;
    out_node->parent_idx=dir->inode_idx;

    if (inode.flags & INODE_DIR) {
        out_node->flags = VFS_DIRECTORY;
    } else {
        out_node->flags = VFS_FILE;
    }

    return 0;
}

// 폴더 경로 토큰 해석
// out_canonical_path 넣을 시 '..'과 '.' 노드의 이름을 실제 폴더 이름으로 변환(선택)
static int resolve_step(vfs_node* cur, const char* token, char* out_canonical_path) {
    vfs_node next;
    if (vfs_lookup(cur, token, &next) != 0) return -1;
    *cur = next;

    // 만약 전체 경로 문자열이 있을 시 폴더 이름이 .. 또는 .으로 되어있는것을 실제 폴더 이름으로 변환
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

// 폴더 경로(path)로 노트 찾아서 out_node에 저장
// out_canonical_path에는 전체 경로가 문자열로 저장됨(선택)
int vfs_resolve_path(const char* path, vfs_node* out_node, char* out_canonical_path) {
    if (!out_node) return -1;

    // path가 없으면 현재 디렉터리
    if (!path || path[0] == '\0') {
        *out_node = current_dir;
        if (out_canonical_path) {
            memcpy(out_canonical_path, current_path, strlen(current_path) + 1);
        }
        return 0;
    }

    vfs_node cur;
    int i = 0;
    
    if (path[0] == '/') {  // 절대 경로
        cur = vfs_root;
        if (out_canonical_path) {
            out_canonical_path[0] = '\0';
        }
        while (path[i] == '/') i++;
    } else {  // 상대 경로
        cur = current_dir;
        if (out_canonical_path) {
            if (strcmp(current_path, "/") == 0) {
                out_canonical_path[0] = '\0';
            } else {
                memcpy(out_canonical_path, current_path, strlen(current_path) + 1);
            }
        }
    }

    // '/' 기준으로 나눠서 토큰마다 해석
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


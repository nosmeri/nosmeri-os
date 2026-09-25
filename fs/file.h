#pragma once
#include "vfs.h"

#define MAX_FD 16

struct File;

// 파일 연산 함수 포인터 테이블
struct FileOps {
    int (*read)(File* file, void* buf, unsigned int count);
    int (*write)(File* file, const void* buf, unsigned int count);
    int (*close)(File* file);
};

enum FileType {
    FILE_TYPE_UNUSED = 0,
    FILE_TYPE_STDIN,
    FILE_TYPE_STDOUT,
    FILE_TYPE_STDERR,
    FILE_TYPE_REGULAR, // VFS 일반 파일
    FILE_TYPE_PIPE     // 향후 파이프용
};

// 열린 파일 객체
struct File {
    FileType type;
    FileOps* ops;
    vfs_node node;       // FILE_TYPE_REGULAR일 때 사용
    unsigned int offset; // 파일 읽기/쓰기 커서
    unsigned int flags;  // 읽기/쓰기 모드
    int ref_count;       // dup2나 fork 시 공유를 위한 참조 카운트
};


File* create_stdin_file();
File* create_stdout_file();
File* create_stderr_file();
File* create_vfs_file(vfs_node* node);

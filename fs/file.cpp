#include "file.h"
#include "keyboard.h"
#include "heap.h"
#include "vfs.h"
#include "vga.h"

// 키보드에서 1문자씩 꺼내 buf를 채움
static int stdin_read(File* file, void* buf, unsigned int count) {
    (void)file;
    char* cbuf = (char*)buf;
    unsigned int bytes_read = 0;

    while (bytes_read < count && buffer_has_char()) {
        char c = dequeue_key();
        if (c == 0) break;
        cbuf[bytes_read++] = c;
    }
    return bytes_read;
}

static FileOps stdin_ops = { stdin_read, 0, 0 };

// 화면에 일반 텍스트 출력
static int stdout_write(File* file, const void* buf, unsigned int count) {
    (void)file;
    const char* cbuf = (const char*)buf;
    for (unsigned int i = 0; i < count; i++) {
        print_char(cbuf[i]);
    }
    return count;
}

static FileOps stdout_ops = { 0, stdout_write, 0 };

// 화면에 빨간색 에러 텍스트 출력 (선택 사항)
static int stderr_write(File* file, const void* buf, unsigned int count) {
    (void)file;
    const char* cbuf = (const char*)buf;
    set_text_color(VGA_COLOR_RED, VGA_COLOR_BLACK);
    for (unsigned int i = 0; i < count; i++) {
        print_char(cbuf[i]);
    }
    set_text_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK); // 색상 원복
    return count;
}

static FileOps stderr_ops = { 0, stderr_write, 0 };

// VFS를 통해 파일 데이터 읽기 & offset 증가
static int vfs_file_read(File* file, void* buf, unsigned int count) {
    int bytes = vfs_read(&file->node, (unsigned char*)buf, count);
    if (bytes > 0) file->offset += bytes;
    return bytes;
}

static int vfs_file_write(File* file, const void* buf, unsigned int count) {
    int bytes = vfs_write(&file->node, buf, count);
    if (bytes > 0) file->offset += bytes;
    return bytes;
}

static int vfs_file_close(File* file) {
    file->type = FILE_TYPE_UNUSED;
    return 0;
}

static FileOps vfs_file_ops = { vfs_file_read, vfs_file_write, vfs_file_close };

File* create_stdin_file() {
    File* f = (File*)kmalloc(sizeof(File));
    f->type = FILE_TYPE_STDIN;
    f->ops = &stdin_ops;
    f->offset = 0;
    f->flags = 0;
    f->ref_count = 1;
    return f;
}

File* create_stdout_file() {
    File* f = (File*)kmalloc(sizeof(File));
    f->type = FILE_TYPE_STDOUT;
    f->ops = &stdout_ops;
    f->offset = 0;
    f->flags = 0;
    f->ref_count = 1;
    return f;
}

File* create_stderr_file() {
    File* f = (File*)kmalloc(sizeof(File));
    f->type = FILE_TYPE_STDERR;
    f->ops = &stderr_ops;
    f->offset = 0;
    f->flags = 0;
    f->ref_count = 1;
    return f;
}

// VFS 파일을 열어서 File 구조체로 감싸 반환
File* create_vfs_file(vfs_node* node) {
    File* f = (File*)kmalloc(sizeof(File));
    f->type = FILE_TYPE_REGULAR;
    f->ops = &vfs_file_ops;
    f->node = *node;     // 구조체 복사
    f->offset = 0;
    f->flags = 0;
    f->ref_count = 1;
    return f;
}

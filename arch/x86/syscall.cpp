#include "syscall.h"
#include "timer.h"
#include "task.h"
#include "file.h"
#include "heap.h"
#include "vfs.h"

// C++ 시스템 콜 핸들러 본체
// boot.asm의 isr80에서 push esp 한 포인터가 regs로 전달됨
extern "C" void syscall_handler(Registers* regs) {
    Task* current_task = get_current_task();
    switch (regs->eax) {
        case SYS_EXIT: {
            // 현재 태스크를 DEAD 상태로 변경
            exit_task();
            break;
        }

        case SYS_SLEEP: {
            // ebx 레지스터에 délay 값이 담겨 전달됨
            unsigned int delay_ticks = regs->ebx;
            task_sleep(delay_ticks);
            break;
        }

        case SYS_YIELD: {
            schedule();
            break;
        }

        case SYS_GETTICK: {
            // 반환값은 eax 레지스터에 저장
            regs->eax = get_tick();
            break;
        }

        case SYS_READ: { // read(int fd, void* buf, unsigned int count)
            int fd = (int)regs->ebx;
            void* buf = (void*)regs->ecx;
            unsigned int count = regs->edx;
            if (fd < 0 || fd >= MAX_FD || !current_task->fd_table[fd]) {
                regs->eax = -1;
                break;
            }
            File* file = current_task->fd_table[fd];
            if (!file->ops || !file->ops->read) {
                regs->eax = -1; // 읽기 미지원 (예: stdout에 read 시도)
                break;
            }
            regs->eax = file->ops->read(file, buf, count);
            break;
        }
        case SYS_WRITE: { // write(int fd, const void* buf, unsigned int count)
            int fd = (int)regs->ebx;
            const void* buf = (const void*)regs->ecx;
            unsigned int count = regs->edx;
            if (fd < 0 || fd >= MAX_FD || !current_task->fd_table[fd]) {
                regs->eax = -1;
                break;
            }
            File* file = current_task->fd_table[fd];
            if (!file->ops || !file->ops->write) {
                regs->eax = -1; // 쓰기 미지원 (예: stdin에 write 시도)
                break;
            }
            regs->eax = file->ops->write(file, buf, count);
            break;
        }
        case SYS_OPEN: {
            char* path = (char*)regs->ebx;
            vfs_node node;
            if (vfs_resolve_path(path, &node) != 0) {
                regs->eax = -1; // open fail
                break;
            }
            File* file = create_vfs_file(&node);
            if (!file) {
                regs->eax = -1;
                break;
            }
            int assigned_fd = -1;
            for (int i = 0; i < MAX_FD; i++) {
                if (!current_task->fd_table[i]) {
                    current_task->fd_table[i] = file;
                    assigned_fd = i;
                    break;
                }
            }
            if (assigned_fd == -1) {
                kfree(file);
                regs->eax = -1;
            } else {
                regs->eax = assigned_fd;
            }
            break;
        }
        case SYS_CLOSE: { // close(int fd)
            int fd = (int)regs->ebx;
            if (fd < 0 || fd >= MAX_FD || !current_task->fd_table[fd]) {
                regs->eax = -1;
                break;
            }
            File* file = current_task->fd_table[fd];
            if (file->ops && file->ops->close) {
                file->ops->close(file); // 내부적으로 ref_count 처리
            } else {
                kfree(file); // 표준 입출력 등은 단순히 해제
            }
            current_task->fd_table[fd] = 0;
            regs->eax = 0; // success
            break;
        }
        case SYS_GETCWD: {
            char* buf = (char*)regs->ebx;
            unsigned int size = regs->ecx;
            if (!buf || size == 0) {
                regs->eax = -1;
                break;
            }
            int len = vfs_getcwd(buf, size);
            if (len < 0) {
                regs->eax = -1;
            } else {
                regs->eax = len;
            }
            break;
        }
        case SYS_CHDIR: {
            char* path = (char*)regs->ebx;
            int ret = vfs_cd(path);
            regs->eax = ret;
            break;
        }
        default:
            break;
    }
}

void sys_exit() {
    __asm__ __volatile__ (
        "int $0x80"
        :
        : "a"(SYS_EXIT)
        : "memory"
    );
    while (true) { }
}

void sys_sleep(unsigned int delay_ticks) {
    __asm__ __volatile__ (
        "int $0x80"
        : 
        : "a"(SYS_SLEEP), "b"(delay_ticks)
        : "memory"
    );
}

void sys_yield() {
    __asm__ __volatile__ (
        "int $0x80"
        : 
        : "a"(SYS_YIELD)
        : "memory"
    );
}

unsigned int sys_get_tick() {
    unsigned int ret;
    __asm__ __volatile__ (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_GETTICK)
        : "memory"
    );
    return ret;
}

int sys_read(int fd, void* buf, unsigned int count) {
    int ret;
    __asm__ __volatile__ (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_READ), "b"(fd), "c"(buf), "d"(count)
        : "memory"
    );
    return ret;
}

int sys_write(int fd, const void* buf, unsigned int count) {
    int ret;
    __asm__ __volatile__ (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_WRITE), "b"(fd), "c"(buf), "d"(count)
        : "memory"
    );
    return ret;
}

int sys_open(const char* path) {
    int ret;
    __asm__ __volatile__ (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_OPEN), "b"(path)
        : "memory"
    );
    return ret;
}

int sys_close(int fd) {
    int ret;
    __asm__ __volatile__ (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_CLOSE), "b"(fd)
        : "memory"
    );
    return ret;
}

int sys_getcwd(char* buf, unsigned int size) {
    int ret;
    __asm__ __volatile__ (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_GETCWD), "b"(buf), "c"(size)
        : "memory"
    );
    return ret;
}

int sys_chdir(const char* path) {
    int ret;
    __asm__ __volatile__ (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_CHDIR), "b"(path)
        : "memory"
    );
    return ret;
}
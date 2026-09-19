#include "shell.h"
#include "vga.h"
#include "timer.h"
#include "string.h"
#include "syscall.h"
#include "pmm.h"
#include "heap.h"
#include "task.h"
#include "keyboard.h"
#include "vfs.h"

static char input_buffer[256];
static int input_buffer_len = 0;

// Forward declarations for command handlers
static void cmd_help(const char* arg);
static void cmd_clear(const char* arg);
static void cmd_sysinfo(const char* arg);
static void cmd_meminfo(const char* arg);
static void cmd_alloc(const char* arg);
static void cmd_test(const char* arg);
static void cmd_fault(const char* arg);
static void cmd_heap(const char* arg);
static void cmd_ps(const char* arg);
static void cmd_kill(const char* arg);
static void cmd_touch(const char* arg);
static void cmd_mkdir(const char* arg);
static void cmd_cd(const char* arg);
static void cmd_ls(const char* arg);
static void cmd_pwd(const char* arg);
static void cmd_write(const char* arg);
static void cmd_cat(const char* arg);
static void cmd_exec(const char* arg);

// 명령어 디스패치 테이블
static const Command commands[] = {
    { "help",    "Show this help menu",                                 cmd_help },
    { "clear",   "Clear the screen",                                    cmd_clear },
    { "sysinfo", "Show system configuration information",               cmd_sysinfo },
    { "meminfo", "Show physical memory usage (PMM)",                    cmd_meminfo },
    { "alloc",   "Test allocating a 4KB physical page",                 cmd_alloc },
    { "test",    "Test sys_print system call (int 0x80)",               cmd_test },
    { "fault",   "Trigger a Page Fault exception (read unmapped addr)", cmd_fault },
    { "heap",    "Display kernel heap memory blocks",                   cmd_heap },
    { "ps",      "List all running tasks/processes",                    cmd_ps },
    { "kill",    "Terminate a task by PID (e.g. kill 1)",               cmd_kill },
    { "touch",   "Create an empty file",                                cmd_touch },
    { "mkdir",   "Create a directory",                                  cmd_mkdir },
    { "cd",      "Change directory",                                    cmd_cd },
    { "ls",      "List directory contents",                             cmd_ls },
    { "pwd",     "Print working directory",                             cmd_pwd },
    { "write",   "Write text to a file (e.g. write file.txt hello)",    cmd_write },
    { "cat",     "Display file content (e.g. cat file.txt)",            cmd_cat },
    { "exec",    "Execute a binary file",                               cmd_exec },
};

static const int num_commands = sizeof(commands) / sizeof(commands[0]);

// 쉘 프롬프트 출력
void print_prompt() {
    char path_buf[256];
    vfs_getcwd(path_buf, sizeof(path_buf));
    print_string(path_buf);
    print_string("$ ");
}

// 디렉터리 엔트리 출력 헬퍼
static void print_dir_entries(const vfs_node* dir) {
    if (!dir) return;
    vfs_node entry;
    for (int i = 0; i < 16; i++) {
        if (vfs_readdir(dir, i, &entry) == 0) {
            print_string(entry.name);
            if (entry.flags & VFS_DIRECTORY) {
                print_string("/");
            }
            print_string("\n");
        }
    }
}

// 각 명령어별 핸들러 함수 구현

static void cmd_help(const char* arg) {
    (void)arg;
    print_string("NOSMERI-OS Command Shell. Available commands:\n");
    for (int i = 0; i < num_commands; i++) {
        print_string("  ");
        print_string(commands[i].name);

        // 출력 정렬을 위한 공백 채우기
        int len = strlen(commands[i].name);
        for (int s = 0; s < 10 - len; s++) {
            print_char(' ');
        }
        print_string("- ");
        print_string(commands[i].description);
        print_string("\n");
    }
}

static void cmd_clear(const char* arg) {
    (void)arg;
    clear_screen();
}

static void cmd_sysinfo(const char* arg) {
    (void)arg;
    print_string("NOSMERI-OS System Information:\n");
    print_string("  OS Name       : NOSMERI-OS\n");
    print_string("  Kernel Version: 1.0.0\n");
    print_string("  Architecture  : x86 (32-bit)\n");
    print_string("  GDT Status    : Initialized (Active)\n");
    print_string("  IDT Status    : Initialized (Active)\n");
    print_string("  Timer Status  : 100Hz PIT Active\n");
}

static void cmd_meminfo(const char* arg) {
    (void)arg;
    char buf[32];
    print_string("Physical Memory Information (PMM):\n");

    print_string("  Total Memory : ");
    itoa(pmm_get_total_memory_kb() / 1024, buf, 10);
    print_string(buf);
    print_string(" MB (");
    itoa(pmm_get_total_pages(), buf, 10);
    print_string(buf);
    print_string(" pages)\n");

    print_string("  Used Memory  : ");
    itoa(pmm_get_used_memory_kb(), buf, 10);
    print_string(buf);
    print_string(" KB (");
    itoa(pmm_get_used_pages(), buf, 10);
    print_string(buf);
    print_string(" pages)\n");

    print_string("  Free Memory  : ");
    itoa(pmm_get_free_memory_kb() / 1024, buf, 10);
    print_string(buf);
    print_string(" MB (");
    itoa(pmm_get_free_pages(), buf, 10);
    print_string(buf);
    print_string(" pages)\n");
}

static void cmd_alloc(const char* arg) {
    (void)arg;
    char buf[32];
    void* page = pmm_alloc_page();
    if (page != 0) {
        print_string("Allocated 4KB Page at Physical Address: 0x");
        itoa((unsigned int)page, buf, 16);
        print_string(buf);
        print_string("\n");
    } else {
        print_string("Failed to allocate page: Out of Memory!\n");
    }
}

static void cmd_test(const char* arg) {
    (void)arg;
    sys_print("System Call Test: int 0x80 successfully executed!\n");
}

static void cmd_fault(const char* arg) {
    (void)arg;
    print_string("Deliberately accessing unmapped address 0xA0000000 to trigger Page Fault...\n");
    volatile unsigned int* bad_ptr = (volatile unsigned int*)0xA0000000;
    unsigned int val = *bad_ptr;
    (void)val;
}

static void cmd_heap(const char* arg) {
    (void)arg;
    heap_dump();
}

static void cmd_ps(const char* arg) {
    (void)arg;
    task_dump();
}

static void cmd_kill(const char* arg) {
    while (*arg == ' ') arg++;
    if (*arg == '\0') {
        print_string("Usage: kill <pid>\n");
        return;
    }
    int pid = atoi(arg);
    if (kill_task((unsigned int)pid) != -1) {
        print_string("Task killed successfully.\n");
    } else {
        print_string("Failed to kill task (invalid PID or kernel task).\n");
    }
}

static void cmd_touch(const char* arg) {
    while (*arg == ' ') arg++;
    if (*arg == '\0') {
        print_string("Usage: touch <filename>\n");
    } else if (vfs_create(&current_dir, arg) != 0) {
        print_string("Failed to create file (already exists or disk full)\n");
    }
}

static void cmd_mkdir(const char* arg) {
    while (*arg == ' ') arg++;
    if (*arg == '\0') {
        print_string("Usage: mkdir <dirname>\n");
    } else if (vfs_mkdir(&current_dir, arg) != 0) {
        print_string("Failed to create directory (already exists or disk full)\n");
    }
}

static void cmd_cd(const char* arg) {
    while (*arg == ' ') arg++;
    if (*arg == '\0') {
        print_string("Usage: cd <path>\n");
    } else if (vfs_cd(arg) != 0) {
        print_string("Directory not found\n");
    }
}

static void cmd_ls(const char* arg) {
    while (*arg == ' ') arg++;
    if (*arg == '\0') {
        print_dir_entries(&current_dir);
    } else {
        vfs_node node;
        if (vfs_resolve_path(arg, &node) != 0 || !(node.flags & VFS_DIRECTORY)) {
            print_string("Directory not found\n");
        } else {
            print_dir_entries(&node);
        }
    }
}

static void cmd_pwd(const char* arg) {
    (void)arg;
    char path_buf[256];
    if (vfs_getcwd(path_buf, sizeof(path_buf)) == 0) {
        print_string(path_buf);
        print_string("\n");
    } else {
        print_string("Error getting current directory\n");
    }
}

static void cmd_write(const char* arg) {
    while (*arg == ' ') arg++;
    char path_buffer[256];
    int i = 0;
    while (*arg != ' ' && *arg != '\0' && i < 255) {
        path_buffer[i++] = *arg++;
    }
    path_buffer[i] = '\0';

    while (*arg == ' ') arg++;
    if (path_buffer[0] == '\0' || *arg == '\0') {
        print_string("Usage: write <file> <text>\n");
        return;
    }
    vfs_node file;
    if (vfs_resolve_path(path_buffer, &file) != 0) {
        print_string("File not found\n");
        return;
    }
    int result = vfs_write(&file, arg, strlen(arg));
    if (result == -1) {
        print_string("Write failed\n");
    }
}

static void cmd_cat(const char* arg) {
    while (*arg == ' ') arg++;
    if (*arg == '\0') {
        print_string("Usage: cat <file>\n");
        return;
    }
    vfs_node node;
    if (vfs_resolve_path(arg, &node) != 0 || (node.flags & VFS_DIRECTORY)) {
        print_string("File not found or is a directory\n");
        return;
    }
    char* buf = (char*)kmalloc(node.size + 1);
    if (!buf) {
        print_string("Out of memory\n");
        return;
    }
    vfs_read(&node, buf, node.size);
    buf[node.size] = '\0';
    print_string(buf);
    print_char('\n');
    kfree(buf);
}

static void cmd_exec(const char* arg) {
    while (*arg == ' ') arg++;
    if (*arg == '\0') {
        print_string("Usage: exec <filepath>\n");
        return;
    }

    Task* t = create_user_process(arg);
    if (t) {
        print_string("Process spawned with PID: ");
        char buf[16];
        itoa(t->id, buf, 10);
        print_string(buf);
        print_string("\n");
    } else {
        print_string("Failed to execute: file not found or load error\n");
    }
}

// 명령어 디스패처
void execute_command(const char* cmd) {
    while (*cmd == ' ') cmd++;
    if (*cmd == '\0') return;

    // 명령어 이름 추출 (첫 번째 단어)
    char cmd_name[32];
    int i = 0;
    while (*cmd != ' ' && *cmd != '\0' && i < 31) {
        cmd_name[i++] = *cmd++;
    }
    cmd_name[i] = '\0';

    // 인자 시작 위치 파싱 (공백 건너뛰기)
    while (*cmd == ' ') cmd++;
    const char* arg = cmd;

    for (int j = 0; j < num_commands; j++) {
        if (strcmp(cmd_name, commands[j].name) == 0) {
            commands[j].handler(arg);
            return;
        }
    }

    print_string("Unknown command: ");
    print_string(cmd_name);
    print_string("\nType 'help' for available commands.\n");
}

// 키 입력 처리 및 버퍼 관리
void shell_handle_key(char c) {
    if (c == '\n') {
        input_buffer[input_buffer_len] = '\0';
        print_char('\n');
        execute_command(input_buffer);

        input_buffer_len = 0;
        print_prompt();
    } else if (c == '\b') {
        if (input_buffer_len > 0) {
            input_buffer_len--;
            print_char('\b');
        }
    } else {
        if (input_buffer_len < 255) {
            input_buffer[input_buffer_len++] = c;
            print_char(c);
        }
    }
}

// 쉘 초기화
void shell_main() {
    input_buffer_len = 0;
    print_string("Welcome to NOSMERI-OS! Type 'help' to see available commands.\n\n");
    print_prompt();
    while (1) {
        if (buffer_has_char()) {
            char c = dequeue_key();
            shell_handle_key(c);
        } else {
            task_yield();
            __asm__ __volatile__ ("hlt");
        }
    }
}
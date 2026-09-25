#include "shell.h"
#include "timer.h"
#include "string.h"
#include "syscall.h"
#include "pmm.h"
#include "heap.h"
#include "task.h"
#include "vfs.h"

static char input_buffer[256];
static int input_buffer_len = 0;

static void shell_print(const char* str) {
    if (!str) return;
    sys_write(1, str, strlen(str)); // 1번 FD (stdout)으로 출력
}
static void shell_print_char(char c) {
    sys_write(1, &c, 1);            // 1번 FD (stdout)으로 1바이트 출력
}
static void shell_error(const char* str) {
    if (!str) return;
    sys_write(2, str, strlen(str)); // 2번 FD (stderr)으로 출력
}

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
    shell_print(path_buf);
    shell_print("$ ");
}

// 디렉터리 엔트리 출력 헬퍼
static void print_dir_entries(const vfs_node* dir) {
    if (!dir) return;
    vfs_node entry;
    for (int i = 0; i < 16; i++) {
        if (vfs_readdir(dir, i, &entry) == 0) {
            shell_print(entry.name);
            if (entry.flags & VFS_DIRECTORY) {
                shell_print("/");
            }
            shell_print("\n");
        }
    }
}

// 각 명령어별 핸들러 함수 구현

static void cmd_help(const char* arg) {
    (void)arg;
    shell_print("NOSMERI-OS Command Shell. Available commands:\n");
    for (int i = 0; i < num_commands; i++) {
        shell_print("  ");
        shell_print(commands[i].name);

        // 출력 정렬을 위한 공백 채우기
        int len = strlen(commands[i].name);
        for (int s = 0; s < 10 - len; s++) {
            shell_print_char(' ');
        }
        shell_print("- ");
        shell_print(commands[i].description);
        shell_print("\n");
    }
}

static void cmd_clear(const char* arg) {
    (void)arg;
    shell_print_char('\f');
}

static void cmd_sysinfo(const char* arg) {
    (void)arg;
    shell_print("NOSMERI-OS System Information:\n");
    shell_print("  OS Name       : NOSMERI-OS\n");
    shell_print("  Kernel Version: 1.0.0\n");
    shell_print("  Architecture  : x86 (32-bit)\n");
    shell_print("  GDT Status    : Initialized (Active)\n");
    shell_print("  IDT Status    : Initialized (Active)\n");
    shell_print("  Timer Status  : 100Hz PIT Active\n");
}

static void cmd_meminfo(const char* arg) {
    (void)arg;
    char buf[32];
    shell_print("Physical Memory Information (PMM):\n");

    shell_print("  Total Memory : ");
    itoa(pmm_get_total_memory_kb() / 1024, buf, 10);
    shell_print(buf);
    shell_print(" MB (");
    itoa(pmm_get_total_pages(), buf, 10);
    shell_print(buf);
    shell_print(" pages)\n");

    shell_print("  Used Memory  : ");
    itoa(pmm_get_used_memory_kb(), buf, 10);
    shell_print(buf);
    shell_print(" KB (");
    itoa(pmm_get_used_pages(), buf, 10);
    shell_print(buf);
    shell_print(" pages)\n");

    shell_print("  Free Memory  : ");
    itoa(pmm_get_free_memory_kb() / 1024, buf, 10);
    shell_print(buf);
    shell_print(" MB (");
    itoa(pmm_get_free_pages(), buf, 10);
    shell_print(buf);
    shell_print(" pages)\n");
}

static void cmd_alloc(const char* arg) {
    (void)arg;
    char buf[32];
    void* page = pmm_alloc_page();
    if (page != 0) {
        shell_print("Allocated 4KB Page at Physical Address: 0x");
        itoa((unsigned int)page, buf, 16);
        shell_print(buf);
        shell_print("\n");
    } else {
        shell_print("Failed to allocate page: Out of Memory!\n");
    }
}

static void cmd_test(const char* arg) {
    (void)arg;
    sys_write(1, "[FD Test] stdout (fd 1) output!\n", 32);
    sys_write(2, "[FD Test] stderr (fd 2) red error output!\n", 42);

}

static void cmd_fault(const char* arg) {
    (void)arg;
    shell_print("Deliberately accessing unmapped address 0xA0000000 to trigger Page Fault...\n");
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
        shell_print("Usage: kill <pid>\n");
        return;
    }
    int pid = atoi(arg);
    if (kill_task((unsigned int)pid) != -1) {
        shell_print("Task killed successfully.\n");
    } else {
        shell_print("Failed to kill task (invalid PID or kernel task).\n");
    }
}

static void cmd_touch(const char* arg) {
    while (*arg == ' ') arg++;
    if (*arg == '\0') {
        shell_print("Usage: touch <filename>\n");
    } else if (vfs_create(&current_dir, arg) != 0) {
        shell_print("Failed to create file (already exists or disk full)\n");
    }
}

static void cmd_mkdir(const char* arg) {
    while (*arg == ' ') arg++;
    if (*arg == '\0') {
        shell_print("Usage: mkdir <dirname>\n");
    } else if (vfs_mkdir(&current_dir, arg) != 0) {
        shell_print("Failed to create directory (already exists or disk full)\n");
    }
}

static void cmd_cd(const char* arg) {
    while (*arg == ' ') arg++;
    if (*arg == '\0') {
        shell_print("Usage: cd <path>\n");
    } else if (vfs_cd(arg) != 0) {
        shell_print("Directory not found\n");
    }
}

static void cmd_ls(const char* arg) {
    while (*arg == ' ') arg++;
    if (*arg == '\0') {
        print_dir_entries(&current_dir);
    } else {
        vfs_node node;
        if (vfs_resolve_path(arg, &node) != 0 || !(node.flags & VFS_DIRECTORY)) {
            shell_print("Directory not found\n");
        } else {
            print_dir_entries(&node);
        }
    }
}

static void cmd_pwd(const char* arg) {
    (void)arg;
    char path_buf[256];
    if (vfs_getcwd(path_buf, sizeof(path_buf)) == 0) {
        shell_print(path_buf);
        shell_print("\n");
    } else {
        shell_print("Error getting current directory\n");
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
        shell_print("Usage: write <file> <text>\n");
        return;
    }
    vfs_node file;
    if (vfs_resolve_path(path_buffer, &file) != 0) {
        shell_print("File not found\n");
        return;
    }
    int result = vfs_write(&file, arg, strlen(arg));
    if (result == -1) {
        shell_print("Write failed\n");
    }
}

static void cmd_cat(const char* arg) {
    while (*arg == ' ') arg++;
    if (*arg == '\0') {
        shell_print("Usage: cat <file>\n");
        return;
    }
    vfs_node node;
    if (vfs_resolve_path(arg, &node) != 0 || (node.flags & VFS_DIRECTORY)) {
        shell_print("File not found or is a directory\n");
        return;
    }
    char* buf = (char*)kmalloc(node.size + 1);
    if (!buf) {
        shell_print("Out of memory\n");
        return;
    }
    vfs_read(&node, buf, node.size);
    buf[node.size] = '\0';
    shell_print(buf);
    shell_print_char('\n');
    kfree(buf);
}

static void cmd_exec(const char* arg) {
    while (*arg == ' ') arg++;
    if (*arg == '\0') {
        shell_print("Usage: exec <filepath>\n");
        return;
    }

    Task* t = create_user_process(arg);
    if (t) {
        shell_print("Process spawned with PID: ");
        char buf[16];
        itoa(t->id, buf, 10);
        shell_print(buf);
        shell_print("\n");
    } else {
        shell_print("Failed to execute: file not found or load error\n");
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

    shell_error("Unknown command: ");
    shell_error(cmd_name);
    shell_print("\nType 'help' for available commands.\n");
}

// 키 입력 처리 및 버퍼 관리
void shell_handle_key(char c) {
    if (c == '\n') {
        input_buffer[input_buffer_len] = '\0';
        shell_print_char('\n');
        execute_command(input_buffer);

        input_buffer_len = 0;
        print_prompt();
    } else if (c == '\b') {
        if (input_buffer_len > 0) {
            input_buffer_len--;
            shell_print_char('\b');
        }
    } else {
        if (input_buffer_len < 255) {
            input_buffer[input_buffer_len++] = c;
            shell_print_char(c);
        }
    }
}

// 쉘 초기화
void shell_main() {
    input_buffer_len = 0;
    shell_print("Welcome to NOSMERI-OS! Type 'help' to see available commands.\n\n");
    print_prompt();
    while (1) {
        char c;
        int bytes = sys_read(0, &c, 1);
        if (bytes > 0) {
            shell_handle_key(c);
        } else {
            task_yield();
            __asm__ __volatile__ ("hlt");
        }
    }
}
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

// 쉘 프롬프트 출력 ([초].[소수]s >)
void print_prompt() {
    /*
    unsigned int tick = get_tick();
    char time_str[20];
    char decimal_str[3];

    unsigned int sec = tick / TIMER_FREQ;
    unsigned int decimal = tick % TIMER_FREQ;

    itoa(sec, time_str, 10);
    print_string(time_str);
    print_char('.');
    if (decimal < 10) print_char('0');
    itoa(decimal, decimal_str, 10);
    print_string(decimal_str);

    print_string("s > ");*/

    print_string(current_path);
    print_string("$ ");
}

void cmd_ls(const vfs_node* dir) {
    if (!dir) return;
    vfs_node entry;
    for (int i = 0; i < 16; i++) {
        // vfs_readdir가 0을 반환하면 유효한 파일/폴더가 있는 슬롯
        if (vfs_readdir(dir, i, &entry) == 0) {
            print_string(entry.name);
            
            // 디렉터리인 경우 끝에 '/'를 붙여줌
            if (entry.flags & VFS_DIRECTORY) {
                print_string("/");
            }
            print_string("\n");
        }
    }
}

// 명령어 실행기
void execute_command(const char* cmd) {
    if (strcmp(cmd, "help") == 0) {
        print_string("NOSMERI-OS Command Shell. Available commands:\n");
        print_string("  help    - Show this help menu\n");
        print_string("  clear   - Clear the screen\n");
        print_string("  sysinfo - Show system configuration information\n");
        print_string("  meminfo - Show physical memory usage (PMM)\n");
        print_string("  alloc   - Test allocating a 4KB physical page\n");
        print_string("  test    - Test sys_print system call (int 0x80)\n");
        print_string("  fault   - Trigger a Page Fault exception (read unmapped address)\n");
        print_string("  heap    - Display kernel heap memory blocks\n");
        print_string("  ps      - List all running tasks/processes\n");
        print_string("  kill    - Terminate a task by PID (e.g. kill 1)\n");
        print_string("  touch   - Create an empty file\n");
        print_string("  mkdir   - Create a directory\n");
        print_string("  cd      - Change directory\n");
        print_string("  ls      - List directory contents\n");
        print_string("  pwd     - Print working directory\n");
        print_string("  write   - Write text to a file (e.g. write file.txt hello)\n");
        print_string("  cat     - Display file content (e.g. cat file.txt)\n");
    } else if (strncmp(cmd, "write ", 6) == 0) {
        const char* arg = cmd + 5;
        while (arg[0] == ' ') arg++;
        char path_buffer[256];
        int i = 0;
        while (arg[0] != ' ' && arg[0] != '\0') path_buffer[i++] = *(arg++);
        path_buffer[i] = '\0';
        while (arg[0] == ' ') arg++;
        if (path_buffer[0] == '\0' || arg[0] == '\0') {
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
    } else if (strcmp(cmd, "write") == 0) {
        print_string("Usage: write <file> <text>\n");
    } else if (strncmp(cmd, "cat ", 4) == 0) {
        const char* arg = cmd + 3;
        while (arg[0] == ' ') arg++;
        if (arg[0] == '\0') {
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
    } else if (strcmp(cmd, "cat") == 0) {
        print_string("Usage: cat <file>\n");
    } else if (strncmp(cmd, "touch ", 6) == 0) {
        const char* arg = cmd + 5;
        while (arg[0] == ' ') arg++;
        if (arg[0] == '\0') {
            print_string("Usage: touch <filename>\n");
        } else if (vfs_create(&current_dir, arg) != 0) {
            print_string("Failed to create file (already exists or disk full)\n");
        }
    } else if (strcmp(cmd, "touch") == 0) {
        print_string("Usage: touch <filename>\n");
    } else if (strncmp(cmd, "mkdir ", 6) == 0) {
        const char* arg = cmd + 5;
        while (arg[0] == ' ') arg++;
        if (arg[0] == '\0') {
            print_string("Usage: mkdir <dirname>\n");
        } else if (vfs_mkdir(&current_dir, arg) != 0) {
            print_string("Failed to create directory (already exists or disk full)\n");
        }
    } else if (strcmp(cmd, "mkdir") == 0) {
        print_string("Usage: mkdir <dirname>\n");
    } else if (strncmp(cmd, "cd ", 3) == 0) {
        const char* arg = cmd + 2;
        while (arg[0] == ' ') arg++;
        if (arg[0] == '\0') {
            print_string("Usage: cd <path>\n");
        } else if (vfs_cd(arg) != 0) {
            print_string("Directory not found\n");
        }
    } else if (strcmp(cmd, "cd") == 0) {
        print_string("Usage: cd <path>\n");
    } else if (strncmp(cmd, "ls ", 3) == 0) {
        const char* arg = cmd + 2;
        while (arg[0] == ' ') arg++;
        vfs_node node;
        if (vfs_resolve_path(arg, &node) != 0 || !(node.flags & VFS_DIRECTORY)) {
            print_string("Directory not found\n");
        } else {
            cmd_ls(&node);
        }
    } else if (strcmp(cmd, "ls") == 0) {
        cmd_ls(&current_dir);
    } else if (strcmp(cmd, "pwd") == 0) {
        print_string(current_path);
        print_string("\n");
    }
    else if (strncmp(cmd, "kill ", 5) == 0) {
        const char* arg = cmd + 5;
        while (*arg == ' ') arg++;

        if (*arg == '\0') {
            print_string("Usage: kill <pid>\n");
        } else {
            int pid = atoi(arg);
            if (kill_task((unsigned int)pid) != -1) {
                print_string("Task killed successfully.\n");
            } else {
                print_string("Failed to kill task (invalid PID or kernel task).\n");
            }
        }
    } else if (strcmp(cmd, "ps") == 0) {
        task_dump();
    } else if (strcmp(cmd, "heap") == 0) {
        heap_dump();
    } else if (strcmp(cmd, "fault") == 0) {
        print_string("Deliberately accessing unmapped address 0xA0000000 to trigger Page Fault...\n");
        // 0~4MB만 매핑되어 있으므로 0xA0000000은 매핑되지 않은 주소 -> Page Fault 발생!
        volatile unsigned int* bad_ptr = (volatile unsigned int*)0xA0000000;
        unsigned int val = *bad_ptr;
        (void)val;
    } else if (strcmp(cmd, "meminfo") == 0) {
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
    } else if (strcmp(cmd, "alloc") == 0) {
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
    } else if (strcmp(cmd, "clear") == 0) {
        clear_screen();
    } else if (strcmp(cmd, "sysinfo") == 0) {
        print_string("NOSMERI-OS System Information:\n");
        print_string("  OS Name       : NOSMERI-OS\n");
        print_string("  Kernel Version: 1.0.0\n");
        print_string("  Architecture  : x86 (32-bit)\n");
        print_string("  GDT Status    : Initialized (Active)\n");
        print_string("  IDT Status    : Initialized (Active)\n");
        print_string("  Timer Status  : 100Hz PIT Active\n");
    } else if (strcmp(cmd, "") == 0) {
        // 빈 줄 입력은 무시
    } else {
        print_string("Unknown command: ");
        print_string(cmd);
        print_string("\nType 'help' for available commands.\n");
    }
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
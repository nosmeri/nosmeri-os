#include "shell.h"
#include "vga.h"
#include "timer.h"
#include "string.h"
#include "syscall.h"
#include "pmm.h"

static char input_buffer[256];
static int input_buffer_len = 0;

// C 스타일 문자열 비교용 strcmp 직접 구현
int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// 쉘 프롬프트 출력 ([초].[소수]s >)
void print_prompt() {
    unsigned int tick = get_tick();
    char time_str[20];
    char decimal_str[3];

    unsigned int sec = tick / 100;
    unsigned int decimal = tick % 100;

    itoa(sec, time_str, 10);
    print_string(time_str);
    print_char('.');
    if (decimal < 10) print_char('0');
    itoa(decimal, decimal_str, 10);
    print_string(decimal_str);

    print_string("s > ");
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
void shell_init() {
    input_buffer_len = 0;
    print_string("Welcome to NOSMERI-OS! Type 'help' to see available commands.\n\n");
    print_prompt();
}

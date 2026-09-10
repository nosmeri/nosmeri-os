unsigned short* const VIDEO_MEMORY = (unsigned short*)0xB8000;
int cursor_pos = 0;

#include "gdt.h"
#include "idt.h"

// 미국 QWERTY 스캔코드 셋 1 매핑 테이블
const char kbd_us[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', /* 9 */
  '9', '0', '-', '=', '\b', /* Backspace */
  '\t',                 /* Tab */
  'q', 'w', 'e', 'r',   /* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', /* Enter key */
    0,                  /* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', /* 39 */
 '\'', '`',   0,        /* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',            /* 49 */
  'm', ',', '.', '/',   0,              /* Right shift */
  '*',
    0,  /* Alt */
  ' ',  /* Space bar */
    0,  /* Caps lock */
    0,  /* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,  /* < ... F10 */
    0,  /* 69 - Num lock*/
    0,  /* Scroll Lock */
    0,  /* Home key */
    0,  /* Up Arrow */
    0,  /* Page Up */
  '-',
    0,  /* Left Arrow */
    0,
    0,  /* Right Arrow */
  '+',
    0,  /* 79 - End key*/
    0,  /* Down Arrow */
    0,  /* Page Down */
    0,  /* Insert Key */
    0,  /* Delete Key */
    0,   0,   0,
    0,  /* F11 Key */
    0,  /* F12 Key */
    0,  /* All other keys are undefined */
};

// 화면 전체를 공백으로 지우는 함수 (80 * 25 = 2000글자)
void clear_screen() {
    for (int i = 0; i < 80 * 25; i++) {
        // 0x07은 배경 검은색, 글씨 밝은 회색 속성 / 0x20은 공백(Space) 문자
        VIDEO_MEMORY[i] = (0x07 << 8) | 0x20;
    }
    cursor_pos = 0;
}

void print_char(char c) {
    if (c == '\n') {
        // 다음 줄 시작 위치로 이동
        cursor_pos = ((cursor_pos / 80) + 1) * 80;
    } else if (c == '\b') {
        // 백스페이스 처리
        if (cursor_pos > 0) {
            cursor_pos--;
            VIDEO_MEMORY[cursor_pos] = (0x07 << 8) | ' ';
        }
    } else {
        VIDEO_MEMORY[cursor_pos] = (0x07 << 8) | c;
        cursor_pos++;
    }

    // 화면 크기 (80 * 25)를 넘어가면 맨 위로 스크롤 대신 리셋
    if (cursor_pos >= 80 * 25) {
        clear_screen();
    }
}

void print_string(const char* str) {
    int i = 0;
    while (str[i] != '\0') {
        print_char(str[i]);
        i++;
    }
}

// C 스타일 문자열 비교용 strcmp 직접 구현
int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// 쉘 입력을 받기 위한 버퍼
char input_buffer[256];
int input_buffer_len = 0;

void execute_command(const char* cmd) {
    if (strcmp(cmd, "help") == 0) {
        print_string("PCR-OS Command Shell. Available commands:\n");
        print_string("  help    - Show this help menu\n");
        print_string("  clear   - Clear the screen\n");
        print_string("  sysinfo - Show system configuration information\n");
    } else if (strcmp(cmd, "clear") == 0) {
        clear_screen();
    } else if (strcmp(cmd, "sysinfo") == 0) {
        print_string("PCR-OS System Information:\n");
        print_string("  OS Name      : PCR-OS\n");
        print_string("  Kernel Version: 1.0.0\n");
        print_string("  Architecture : x86 (32-bit)\n");
        print_string("  GDT Status   : Initialized (Active)\n");
        print_string("  IDT Status   : Initialized (Active)\n");
    } else if (strcmp(cmd, "") == 0) {
        // 빈 명령어 입력 시 줄 바꿈만 처리
    } else {
        print_string("Unknown command: ");
        print_string(cmd);
        print_string("\nType 'help' for available commands.\n");
    }
}

extern "C" void handle_keyboard_input(unsigned char scancode) {
    // 키가 떨어지는 이벤트는 무시
    if (scancode & 0x80) {
        return;
    }
    
    if (scancode < 128) {
        char c = kbd_us[scancode];
        if (c != 0) {
            if (c == '\n') {
                // 엔터 입력 시 커맨드 종료
                input_buffer[input_buffer_len] = '\0';
                print_char('\n');
                execute_command(input_buffer);
                
                // 버퍼 초기화 및 프롬프트 재출력
                input_buffer_len = 0;
                print_string("> ");
            } else if (c == '\b') {
                // 백스페이스 입력 시 버퍼에 내용이 있을 때만 삭제
                if (input_buffer_len > 0) {
                    input_buffer_len--;
                    print_char('\b');
                }
            } else {
                // 일반 문자 입력 시 버퍼가 가득 차지 않았으면 추가
                if (input_buffer_len < 255) {
                    input_buffer[input_buffer_len++] = c;
                    print_char(c);
                }
            }
        }
    }
}

extern "C" void kernel_main() {
    clear_screen();
    
    print_string("Initializing GDT...");
    init_gdt();
    print_string(" Done.\n");

    print_string("Initializing IDT...");
    init_idt();
    print_string(" Done.\n");

    // CPU 인터럽트 활성화
    asm volatile("sti");
    print_string("Interrupts enabled.\n");
    print_string("Welcome to PCR-OS! Type 'help' to see available commands.\n\n> ");
    
    while (true) {}
}
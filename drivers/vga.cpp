#include "vga.h"
#include "io.h"

static unsigned short* const VIDEO_MEMORY = (unsigned short*)0xB8000;
static int cursor_pos = 0;
static unsigned char text_attribute = 0x07; // 기본: 검은 배경(0), 밝은 회색 글자(7)

// VGA 하드웨어 커서 위치 갱신 (CRTC 0x3D4, 0x3D5 포트 사용)
void update_cursor(short pos) {
    outb(0x3D4, 0x0F);                  // 커서 위치 하위 바이트 레지스터
    outb(0x3D5, (unsigned char)(pos & 0xFF));
    outb(0x3D4, 0x0E);                  // 커서 위치 상위 바이트 레지스터
    outb(0x3D5, (unsigned char)((pos >> 8) & 0xFF));
}

// 텍스트 전경/배경 색상 설정
void set_text_color(unsigned char fg, unsigned char bg) {
    text_attribute = (bg << 4) | (fg & 0x0F);
}

int get_cursor_pos() {
    return cursor_pos;
}

// 화면 전체를 공백으로 지우는 함수
void clear_screen() {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        VIDEO_MEMORY[i] = (text_attribute << 8) | ' ';
    }
    cursor_pos = 0;
    update_cursor(cursor_pos);
}

void scroll_screen() {
    // 마지막 줄을 제외한 모든 줄을 한 줄씩 위로 올림
    for (int i = 0; i < VGA_WIDTH * (VGA_HEIGHT - 1); i++) {
        VIDEO_MEMORY[i] = VIDEO_MEMORY[i + VGA_WIDTH];
    }

    // 마지막 줄을 공백으로 채움
    for (int i = VGA_WIDTH * (VGA_HEIGHT - 1); i < VGA_WIDTH * VGA_HEIGHT; i++) {
        VIDEO_MEMORY[i] = (text_attribute << 8) | ' ';
    }

    // 커서를 마지막 줄의 처음으로 이동
    cursor_pos = VGA_WIDTH * (VGA_HEIGHT - 1);
    update_cursor(cursor_pos);
}

// VGA 초기화
void vga_init() {
    clear_screen();
}

// 단일 문자 출력 함수
void print_char(char c) {
    if (c == '\n') {
        // 다음 줄 시작 위치로 이동
        cursor_pos = ((cursor_pos / VGA_WIDTH) + 1) * VGA_WIDTH;
    } else if (c == '\b') {
        // 백스페이스 처리
        if (cursor_pos > 0) {
            cursor_pos--;
            VIDEO_MEMORY[cursor_pos] = (text_attribute << 8) | ' ';
        }
    } else {
        VIDEO_MEMORY[cursor_pos] = (text_attribute << 8) | c;
        cursor_pos++;
    }

    // 화면 끝(80 * 25)을 넘어가면 화면 초기화 (추후 스크롤 기능 추가 가능)
    if (cursor_pos >= VGA_WIDTH * VGA_HEIGHT) {
        scroll_screen();
    }
    update_cursor(cursor_pos);
}

// 문자열 출력 함수
void print_string(const char* str) {
    int i = 0;
    while (str[i] != '\0') {
        print_char(str[i]);
        i++;
    }
}

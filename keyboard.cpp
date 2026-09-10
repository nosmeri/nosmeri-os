#include "keyboard.h"
#include "io.h"
#include "pic.h"
#include "shell.h"

// 미국 QWERTY 스캔코드 셋 1 매핑 테이블
static const char kbd_us[128] = {
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

void keyboard_init() {
    // 키보드 IRQ 활성화는 PIC 리매핑 단계에서 이미 unmask됨
}

// C++ 키보드 스캔코드 처리부
extern "C" void handle_keyboard_input(unsigned char scancode) {
    // 키 릴리즈(Key Release: 비트 7 세팅) 이벤트는 무시
    if (scancode & 0x80) {
        return;
    }

    if (scancode < 128) {
        char c = kbd_us[scancode];
        if (c != 0) {
            shell_handle_key(c);
        }
    }
}

// IRQ1 핸들러 (어셈블리 irq1에서 호출)
extern "C" void keyboard_handler() {
    // 키보드 데이터 포트 0x60에서 스캔코드 읽기
    unsigned char scancode = inb(0x60);

    handle_keyboard_input(scancode);

    // PIC에 EOI 전송 (IRQ 1)
    pic_send_eoi(1);
}

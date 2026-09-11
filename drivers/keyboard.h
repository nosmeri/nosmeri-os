#pragma once

// C++ 키보드 스캔코드 처리기
extern "C" void handle_keyboard_input(unsigned char scancode);

// IRQ1 핸들러 (어셈블리 irq1에서 호출)
extern "C" void keyboard_handler();

#pragma once

// IRQ1 핸들러 (어셈블리 irq1에서 호출)
extern "C" void keyboard_handler();

char dequeue_key();
int buffer_has_char();
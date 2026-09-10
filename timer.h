#pragma once

// PIT (Programmable Interval Timer) 초기화 및 틱/딜레이 제어
void init_pit_timer(unsigned int freq);
unsigned int get_tick();
void sleep(unsigned int ms);

// IRQ0 핸들러 (어셈블리 irq0에서 호출)
extern "C" void timer_handler();

#include "timer.h"
#include "io.h"
#include "pic.h"
#include "task.h"

static volatile unsigned int timer_ticks = 0;

// PIT 8254 채널 0 초기화 (기본 클럭: 1193182 Hz)
void init_pit_timer() {
    unsigned int divisor = 1193182 / TIMER_FREQ;
    // 제어 워드 포트 0x43: 채널 0, lobyte/hibyte 액세스, 모드 3(Square wave), 16비트 바이너리
    outb(0x43, 0x36);
    outb(0x40, (unsigned char)(divisor & 0xFF));
    outb(0x40, (unsigned char)((divisor >> 8) & 0xFF));
}

// 틱 수 반환 (100Hz 기준 1 tick = 10ms)
unsigned int get_tick() {
    return timer_ticks;
}

// 밀리초 단위 딜레이 함수 (100Hz = 10ms 단위 해상도)
void sleep(unsigned int ms) {
    unsigned int target_ticks = timer_ticks + (ms * TIMER_FREQ) / 1000;
    while (timer_ticks < target_ticks) {
        // HLT 명령어로 인터럽트가 올 때까지 CPU 저전력 대기
        __asm__ __volatile__ ("hlt");
    }
}

// IRQ0 타이머 인터럽트 핸들러 (100Hz 주기 호출)
extern "C" void timer_handler() {
    timer_ticks++;

    // PIC에 EOI 전송 (IRQ 0)
    pic_send_eoi(0);

    schedule();
}

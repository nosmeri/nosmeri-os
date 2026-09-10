#include "idt.h"

volatile unsigned int timer_ticks = 0;

idt_entry idt_entries[256];
idt_ptr   idt_record;

// 어셈블리에서 정의할 IDT 로드 함수와 예외 핸들러 선언
extern "C" void idt_load(unsigned int);
extern "C" void isr0(); // Division by zero 핸들러
extern "C" void irq0(); // 타이머 인터럽트 핸들러
extern "C" void irq1(); // 키보드 인터럽트 핸들러

// I/O 포트 제어용 인라인 어셈블리 함수
inline void outb(unsigned short port, unsigned char val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// IDT 엔트리 설정 함수
void set_idt_gate(unsigned char num, unsigned int base, unsigned short sel, unsigned char flags) {
    idt_entries[num].base_low  = (base & 0xFFFF);
    idt_entries[num].base_high = (base >> 16) & 0xFFFF;
    idt_entries[num].selector  = sel;
    idt_entries[num].always0   = 0;
    // 0x8E: 세그먼트 존재(Present), Ring 0(Kernel), 32-bit Interrupt Gate
    idt_entries[num].flags     = flags; 
}

// PIC 재매핑 함수
void pic_remap() {
    // 0x2_ : 마스터, 0xA_ : 슬레이브
    outb(0x20, 0x11); // ICW1: 초기화 명령
    outb(0xA0, 0x11);

    outb(0x21, 0x20); // ICW2: 마스터 PIC를 인터럽트 32(0x20)번으로 매핑
    outb(0xA1, 0x28); // ICW2: 슬레이브 PIC를 인터럽트 40(0x28)번으로 매핑

    outb(0x21, 0x04); // ICW3: 마스터와 슬레이브 연결 설정
    outb(0xA1, 0x02);

    outb(0x21, 0x01); // ICW4: 8086 모드 설정
    outb(0xA1, 0x01);

    outb(0x21, 0xFC); // IRQ 0(타이머) 및 IRQ 1(키보드) 활성화 (0xFC = 1111 1100)
    outb(0xA1, 0xFF); // 슬레이브 PIC의 모든 인터럽트 마스크 (0xFF = 1111 1111)
}

void init_pit_timer(unsigned int freq) {
    unsigned int divisor = 1193182 / freq;
    outb(0x43, 0x36);
    outb(0x40, (unsigned char)(divisor & 0xFF));
    outb(0x40, (unsigned char)(divisor >> 8));
}

extern "C" void handle_keyboard_input(unsigned char scancode);

extern "C" void timer_handler() {
    timer_ticks++;
    // PIC에 EOI(인터럽트 종료) 전송
    // 마스터 PIC 명령 포트: 0x20, EOI 값: 0x20
    outb(0x20, 0x20);
}

unsigned int get_tick() {
    return timer_ticks;
}

extern "C" void keyboard_handler() {
    // 키보드 데이터 포트 0x60에서 스캔코드를 읽음
    unsigned char scancode = inb(0x60);
    
    // C++ 커널 처리부 호출
    handle_keyboard_input(scancode);
    
    // PIC에 EOI(인터럽트 종료) 전송
    // 마스터 PIC 명령 포트: 0x20, EOI 값: 0x20
    outb(0x20, 0x20);
}

void init_idt() {
    idt_record.limit = (sizeof(idt_entry) * 256) - 1;
    idt_record.base  = (unsigned int)&idt_entries;

    // 전체 IDT를 0으로 초기화
    for (int i = 0; i < 256; i++) {
        set_idt_gate(i, 0, 0, 0);
    }

    // PIC 인터럽트 번호 재설정
    // 기존의 0x08은 cpu 내부 인터럽트랑 겹침 -> 0x20으로 변경
    pic_remap();

    // 0번 인터럽트(Divide by Zero)에 핸들러 등록
    set_idt_gate(0, (unsigned int)isr0, 0x08, 0x8E);

    // 32번 인터럽트(IRQ 0, 타이머)에 핸들러 등록
    set_idt_gate(32, (unsigned int)irq0, 0x08, 0x8E);
    init_pit_timer(100);

    // 33번 인터럽트(IRQ 1, 키보드)에 핸들러 등록
    set_idt_gate(33, (unsigned int)irq1, 0x08, 0x8E);

    // CPU에 IDT 주소 로드
    idt_load((unsigned int)&idt_record);
}

// C++ 예외 핸들러 실제 구현체
extern "C" void isr_handler() {
    // 화면에 빨간 경고 글씨 등으로 인터럽트가 걸렸음을 표시할 수 있음
    unsigned short* const VIDEO_MEMORY = (unsigned short*)0xB8000;
    VIDEO_MEMORY[80 * 2] = (0x0C << 8) | 'E'; // 빨간색 'E' 출력 (Error)
    VIDEO_MEMORY[80 * 2 + 1] = (0x0C << 8) | 'R';
    VIDEO_MEMORY[80 * 2 + 2] = (0x0C << 8) | 'R';
    
    while (true); // 시스템 정지
}
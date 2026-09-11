#include "idt.h"
#include "pic.h"
#include "timer.h"
#include "keyboard.h"
#include "vga.h"

idt_entry idt_entries[256];
idt_ptr   idt_record;

// 어셈블리에서 정의된 저수준 핸들러 선언
extern "C" void idt_load(unsigned int);
extern "C" void isr0(); // Division by zero 핸들러
extern "C" void isr14(); // Page fault 핸들러
extern "C" void irq0(); // 타이머 인터럽트 핸들러
extern "C" void irq1(); // 키보드 인터럽트 핸들러
extern "C" void isr80(); // 시스템 콜 인터럽트 핸들러

// IDT 엔트리 설정 함수
void set_idt_gate(unsigned char num, unsigned int base, unsigned short sel, unsigned char flags) {
    idt_entries[num].base_low  = (base & 0xFFFF);
    idt_entries[num].base_high = (base >> 16) & 0xFFFF;
    idt_entries[num].selector  = sel;
    idt_entries[num].always0   = 0;
    // 0x8E: 세그먼트 존재(Present), Ring 0(Kernel), 32-bit Interrupt Gate
    idt_entries[num].flags     = flags;
}

// IDT 전체 초기화 함수
void init_idt() {
    idt_record.limit = (sizeof(idt_entry) * 256) - 1;
    idt_record.base  = (unsigned int)&idt_entries;

    // 전체 IDT를 0으로 초기화
    for (int i = 0; i < 256; i++) {
        set_idt_gate(i, 0, 0, 0);
    }

    // PIC 인터럽트 오프셋 재설정 (Master: 0x20, Slave: 0x28)
    pic_remap();

    // 0번 예외(Divide by Zero)에 핸들러 등록
    set_idt_gate(0, (unsigned int)isr0, 0x08, 0x8E);

    // 14번 예외(Page Fault)에 핸들러 등록
    set_idt_gate(14, (unsigned int)isr14, 0x08, 0x8E);

    // 32번 인터럽트(IRQ 0, 타이머)에 핸들러 등록 및 100Hz 타이머 활성화
    set_idt_gate(32, (unsigned int)irq0, 0x08, 0x8E);
    init_pit_timer(100);

    // 33번 인터럽트(IRQ 1, 키보드)에 핸들러 등록
    set_idt_gate(33, (unsigned int)irq1, 0x08, 0x8E);

    // 0x80번 인터럽트(시스템 콜): 0xEE (Present=1, DPL=3, Type=Interrupt Gate) 유저 모드 호출 허용
    set_idt_gate(0x80, (unsigned int)isr80, 0x08, 0xEE);

    // CPU에 IDT 주소 로드
    idt_load((unsigned int)&idt_record);
}

// C++ 예외 핸들러 구현체 (Division by zero 등 발생 시)
extern "C" void isr_handler() {
    set_text_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    print_string("\n[EXCEPTION] CPU Divide by Zero Exception (ISR0)!\n");
    print_string("System halted.\n");

    while (true) {
        __asm__ __volatile__ ("hlt");
    }
}
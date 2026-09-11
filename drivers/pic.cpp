#include "pic.h"
#include "io.h"

// 8259 PIC 재매핑 함수
void pic_remap() {
    // ICW1: 초기화 명령 (0x11 = 에지 트리거, 캐스케이드 모드, ICW4 필요)
    outb(PIC1_COMMAND, 0x11);
    io_wait();
    outb(PIC2_COMMAND, 0x11);
    io_wait();

    // ICW2: 마스터 PIC를 인터럽트 32(0x20)번, 슬레이브 PIC를 40(0x28)번으로 오프셋 재설정
    outb(PIC1_DATA, 0x20);
    io_wait();
    outb(PIC2_DATA, 0x28);
    io_wait();

    // ICW3: 마스터 IRQ2 라인에 슬레이브 연결 설정
    outb(PIC1_DATA, 0x04);
    io_wait();
    outb(PIC2_DATA, 0x02);
    io_wait();

    // ICW4: 8086/88 모드 활성화
    outb(PIC1_DATA, 0x01);
    io_wait();
    outb(PIC2_DATA, 0x01);
    io_wait();

    // 초기 마스크 설정: IRQ0(타이머) 및 IRQ1(키보드) 활성화 (0xFC = 0b11111100)
    outb(PIC1_DATA, 0b11111100);
    outb(PIC2_DATA, 0b11111111); // 슬레이브 PIC 전체 마스크
}

// EOI (End of Interrupt) 전송 함수
void pic_send_eoi(unsigned char irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

// 특정 IRQ 마스크 (비활성화)
void pic_set_mask(unsigned char irq_line) {
    unsigned short port;
    unsigned char value;

    if (irq_line < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq_line -= 8;
    }
    value = inb(port) | (1 << irq_line);
    outb(port, value);
}

// 특정 IRQ 마스크 해제 (활성화)
void pic_clear_mask(unsigned char irq_line) {
    unsigned short port;
    unsigned char value;

    if (irq_line < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq_line -= 8;
    }
    value = inb(port) & ~(1 << irq_line);
    outb(port, value);
}

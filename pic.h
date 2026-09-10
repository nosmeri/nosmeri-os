#pragma once

// PIC 기본 I/O 포트
#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

// PIC EOI (End of Interrupt) 명령 코드
#define PIC_EOI      0x20

// 함수 선언
void pic_remap();
void pic_send_eoi(unsigned char irq);
void pic_set_mask(unsigned char irq_line);
void pic_clear_mask(unsigned char irq_line);

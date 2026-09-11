#pragma once

// 컴파일러 패딩 방지
struct idt_entry {
    unsigned short base_low;    // ISR 함수의 하위 16비트 주소
    unsigned short selector;    // 커널 코드 세그먼트 오프셋 (GDT의 커널 코드 = 0x08)
    unsigned char  always0;     // 항상 0으로 채워야 하는 바이트
    unsigned char  flags;       // 권한 및 게이트 타입 설정 (P, DPL, Type 등)
    unsigned short base_high;   // ISR 함수의 상위 16비트 주소
} __attribute__((packed));

// IDTR 레지스터에 로드할 6바이트 포인터 구조체
struct idt_ptr {
    unsigned short limit;       // IDT 테이블 크기 - 1
    unsigned int   base;        // IDT 테이블의 물리 주소
} __attribute__((packed));

// IDT 관련 함수 선언
void set_idt_gate(unsigned char num, unsigned int base, unsigned short sel, unsigned char flags);
void init_idt();
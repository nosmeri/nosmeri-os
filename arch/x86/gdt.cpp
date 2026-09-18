#include "gdt.h"
#include "string.h"

// 변수 선언
gdt_entry gdt_entries[6];
gdt_ptr   gdt_record;

// 어셈블리 파일에서 호출할 외부 함수 선언
extern "C" void gdt_flush(unsigned int);
extern "C" void tss_flush(unsigned int);

static tss_entry tss;

// 유저모드에서 인터럽트 발생시 변경할 커널 스택 포인터를 지정하는 함수
void set_kernel_stack(unsigned int stack_top) {
    tss.esp0 = stack_top;
}


// 개별 GDT 엔트리를 설정하는 헬퍼 함수
void set_gdt_gate(int num, unsigned int base, unsigned int limit, unsigned char access, unsigned char gran) {
    gdt_entries[num].base_low    = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high   = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low   = (limit & 0xFFFF);
    gdt_entries[num].granularity = (limit >> 16) & 0x0F;

    gdt_entries[num].granularity |= (gran & 0xF) << 4;
    gdt_entries[num].access      = access;
}

// task state segment(TSS) 초기화
void init_tss() {
    memset(&tss, 0, sizeof(tss_entry));
    tss.ss0 = 0x10; // Ring 0 Data Segment
    tss.esp0 = 0x800000; // Ring 0 Stack Pointer (Initial)
    tss.iomap_base = sizeof(tss_entry); // I/O 포트 접근 권한 테이블의 시작 주소
    // 끝 주소로 두면 아무 포트도 접근 못함

    set_gdt_gate(5, (unsigned int)&tss, sizeof(tss_entry) - 1, 0x89, 0);
    /*
    Present = 1
    DPL = 00 (Ring 0만 접근 가능)
    S = 0 (시스템 세그먼트)
    Type = 1001 (32-bit TSS Available)
    */
}

// GDT 전체를 초기화하고 CPU에 로드하는 함수
void init_gdt() {
    gdt_record.limit = (sizeof(gdt_entry) * 6) - 1;
    gdt_record.base  = (unsigned int)&gdt_entries;

    // (0b1100)
    // Limit을 4KB 단위, 명령어를 32비트로 처리

    // 1. Null Descriptor
    set_gdt_gate(0, 0, 0, 0, 0);
    // 2. Kernel Code Segment (실행 가능, 읽기 가능, Ring 0)
    set_gdt_gate(1, 0, 0xFFFFF, 0b10011010, 0b1100);
    // 3. Kernel Data Segment (쓰기 가능, 읽기 가능, Ring 0)
    set_gdt_gate(2, 0, 0xFFFFF, 0b10010010, 0b1100);
    // 4. User Code Segment   (실행 가능, 읽기 가능, Ring 3)
    set_gdt_gate(3, 0, 0xFFFFF, 0b11111010, 0b1100);
    // 5. User Data Segment   (쓰기 가능, 읽기 가능, Ring 3)
    set_gdt_gate(4, 0, 0xFFFFF, 0b11110010, 0b1100);

    init_tss();

    // CPU에 GDT 주소를 등록하고 세그먼트 레지스터를 리로드
    gdt_flush((unsigned int)&gdt_record);
    
    // TR 레지스터에 TSS 셀렉터(인덱스 5번)를 로드
    tss_flush(0x28); // 인덱스 5 * 8 = 40 = 0x28
}
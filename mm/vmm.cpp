#include "vmm.h"
#include "vga.h"
#include "string.h"

__attribute__((aligned(4096))) static unsigned int page_directory[1024];
__attribute__((aligned(4096))) static unsigned int first_page_table[1024];

void init_vmm() {
    for (int i = 0; i < 1024; i++) {
        page_directory[i] = 0;
    }
    page_directory[0] = (unsigned int) first_page_table | PAGE_PRESENT | PAGE_RW;

    for (int i = 0; i < 1024; i++) {
        first_page_table[i] = (unsigned int)(i * 0x1000) | PAGE_PRESENT | PAGE_RW;
    }
    
    // CR3 레지스터에 페이지 디렉토리 주소 설정
    asm volatile("mov %0, %%cr3" : : "r"(page_directory));
    
    // 페이징 활성화 (CR0 레지스터의 PG 비트를 1로 설정)
    unsigned int cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; // PG 비트 설정
    asm volatile("mov %0, %%cr0" : : "r"(cr0));
}

// 14번 Page Fault 예외 핸들러
extern "C" void page_fault_handler(unsigned int error_code) {
    // CR2 레지스터에 잘못된 접근이 발생한 가상 주소가 담겨있음
    unsigned int faulting_address;
    asm volatile("mov %%cr2, %0" : "=r"(faulting_address));

    set_text_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    print_string("\n========================================\n");
    print_string(" [KERNEL PANIC] PAGE FAULT EXCEPTION!\n");
    print_string("========================================\n");

    char buf[32];
    print_string("Faulting Virtual Address: 0x");
    itoa(faulting_address, buf, 16);
    print_string(buf);
    print_string("\n");

    print_string("Error Code Details:\n");
    // Bit 0: Present (0 = 페이지 없음, 1 = 보호 위반)
    if (!(error_code & 0x1)) {
        print_string("  - Cause       : Super/Non-present page (Page not present in RAM)\n");
    } else {
        print_string("  - Cause       : Page-protection violation\n");
    }

    // Bit 1: Write (0 = 읽기 시도, 1 = 쓰기 시도)
    if (error_code & 0x2) {
        print_string("  - Access Type : Write operation\n");
    } else {
        print_string("  - Access Type : Read operation\n");
    }

    // Bit 2: User (0 = 커널 모드, 1 = 유저 모드)
    if (error_code & 0x4) {
        print_string("  - Privilege   : User-mode (Ring 3)\n");
    } else {
        print_string("  - Privilege   : Kernel-mode (Ring 0)\n");
    }

    print_string("System halted.\n");

    while (true) {
        asm volatile("hlt");
    }
}
#include "gdt.h"
#include "idt.h"
#include "vga.h"
#include "shell.h"
#include "pmm.h"

extern "C" void kernel_main(unsigned int magic, multiboot_info* mbi) {
    // 1. 화면 드라이버 초기화
    vga_init();

    // 2. GDT 초기화
    print_string("Initializing GDT...");
    init_gdt();
    print_string(" Done.\n");

    // 3. IDT 및 하드웨어(PIC, PIT, 키보드) 인터럽트 초기화
    print_string("Initializing IDT & Interrupts...");
    init_idt();
    print_string(" Done.\n");

    // 4. 물리 메모리 관리자(PMM) 초기화
    print_string("Initializing Physical Memory Manager...");
    init_pmm(magic, mbi);
    print_string(" Done.\n");

    // 5. CPU 인터럽트 활성화
    __asm__ __volatile__ ("sti");
    print_string("Interrupts enabled.\n\n");

    // 6. 쉘 시작
    shell_init();

    // 6. Idle loop
    while (true) {
        __asm__ __volatile__ ("hlt");
    }
}
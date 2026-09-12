#include "gdt.h"
#include "idt.h"
#include "vga.h"
#include "shell.h"
#include "pmm.h"
#include "vmm.h"
#include "heap.h"
#include "task.h"

extern "C" void kernel_main(unsigned int magic, multiboot_info* mbi) {
    // 1. 화면 드라이버 초기화
    vga_init();

    // GDT 초기화
    print_string("Initializing GDT...");
    init_gdt();
    print_string(" Done.\n");

    // IDT 및 하드웨어(PIC, PIT, 키보드) 인터럽트 초기화
    print_string("Initializing IDT & Interrupts...");
    init_idt();
    print_string(" Done.\n");

    // 물리 메모리 관리자(PMM) 초기화
    print_string("Initializing Physical Memory Manager...");
    init_pmm(magic, mbi);
    print_string(" Done.\n");

    // 가상 메모리 관리자(VMM) 초기화
    print_string("Initializing Virtual Memory Manager...");
    init_vmm();
    print_string(" Done.\n");

    // 가상 힙 초기화
    print_string("Initializing Heap...");
    init_heap();
    print_string(" Done.\n");

    // 멀티태스킹 초기화
    print_string("Initializing Task...");
    init_tasking();
    print_string(" Done.\n");

    // CPU 인터럽트 활성화
    __asm__ __volatile__ ("sti");
    print_string("Interrupts enabled.\n\n");

    // 쉘 시작
    shell_init();

    // Idle loop
    while (true) {
        __asm__ __volatile__ ("hlt");
    }
}
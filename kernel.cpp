#include "gdt.h"
#include "idt.h"
#include "vga.h"
#include "shell.h"
#include "pmm.h"
#include "vmm.h"
#include "heap.h"
#include "task.h"
#include "vfs.h"

void spinner() {
    volatile unsigned short* vga = (volatile unsigned short*)0xB8000;
    const char spinner[] = {'|', '/', '-', '\\'};
    int idx = 0;
    while (true) {
        // 우측 상단 (0행 79열)에 녹색 스피너 표시
        vga[79] = (unsigned short)(spinner[idx % 4] | (0x0A << 8));
        idx++;
        // 약간의 딜레이
        task_sleep(100);
    }
}

void user_function() {
    
    const char* msg = "[User Task] Direct int 0x80 called!\n";
    __asm__ __volatile__ (
        "int $0x80"
        :
        : "a"(1), "b"(msg)   // eax = 1 (SYS_PRINT), ebx = 문자열 포인터
        : "memory"
    );

    __asm__ __volatile__ (
        "int $0x80"
        :
        : "a"(4), "b"(1000)   // eax = 4 (SYS_SLEEP), ebx = 1000ms
        : "memory"
    );

    const char* msg2 = "[User Task] Exit Task\n";
    __asm__ __volatile__ (
        "int $0x80"
        :
        : "a"(1), "b"(msg2)   // eax = 1 (SYS_PRINT), ebx = 문자열 포인터
        : "memory"
    );

    user_exit();

    const char* msg3 = "[User Task] Exit Check\n";
    __asm__ __volatile__ (
        "int $0x80"
        :
        : "a"(1), "b"(msg3)   // eax = 1 (SYS_PRINT), ebx = 문자열 포인터
        : "memory"
    );

    // 유저 모드에서 금지된 명령어 시도 
    //__asm__ __volatile__ ("cli");

    while (true) { };
}

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
    create_task(spinner);
    create_user_task(user_function);
    print_string(" Done.\n");

    print_string("Initializing VFS...");
    vfs_init();
    print_string(" Done.\n");

    // CPU 인터럽트 활성화
    __asm__ __volatile__ ("sti");
    print_string("Interrupts enabled.\n\n");

    // 쉘 시작
    shell_main();

}
#include "gdt.h"
#include "idt.h"
#include "vga.h"
#include "shell.h"
#include "pmm.h"
#include "vmm.h"
#include "heap.h"
#include "task.h"
#include "vfs.h"
/*
static const unsigned char sample_hello_bin[] = {
    0xB8, 0x01, 0x00, 0x00, 0x00, 0xBB, 0x25, 0x00, 0x00, 0x40, 0xCD, 0x80,
    0xB8, 0x04, 0x00, 0x00, 0x00, 0xBB, 0xE8, 0x03, 0x00, 0x00, 0xCD, 0x80,
    0xB8, 0x01, 0x00, 0x00, 0x00, 0xBB, 0x49, 0x00, 0x00, 0x40, 0xCD, 0x80,
    0xC3, // ret
    // 문자열 1: "[Disk Exec] Hello from /hello.bin!\n\0"
    0x5B, 0x44, 0x69, 0x73, 0x6B, 0x20, 0x45, 0x78, 0x65, 0x63, 0x5D, 0x20,
    0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x66, 0x72, 0x6F, 0x6D, 0x20, 0x2F,
    0x68, 0x65, 0x6C, 0x6C, 0x6F, 0x2E, 0x62, 0x69, 0x6E, 0x21, 0x0A, 0x00,
    // 문자열 2: "[Disk Exec] Finished after sleep!\n\0"
    0x5B, 0x44, 0x69, 0x73, 0x6B, 0x20, 0x45, 0x78, 0x65, 0x63, 0x5D, 0x20,
    0x46, 0x69, 0x6E, 0x69, 0x73, 0x68, 0x65, 0x64, 0x20, 0x61, 0x66, 0x74,
    0x65, 0x72, 0x20, 0x73, 0x6C, 0x65, 0x65, 0x70, 0x21, 0x0A, 0x00
};

static const unsigned char sample_bad_bin[] = {
    0xB8, 0x01, 0x00, 0x00, 0x00, 0xBB, 0x0E, 0x00, 0x00, 0x40, 0xCD, 0x80,
    0xFA, // cli 명령어 (Ring 3 실행 불가!)
    0xC3, // ret
    // 메시지: "[Privilege Test] Ring 3 attempting CLI instruction...\n\0"
    0x5B, 0x50, 0x72, 0x69, 0x76, 0x69, 0x6C, 0x65, 0x67, 0x65, 0x20, 0x54,
    0x65, 0x73, 0x74, 0x5D, 0x20, 0x52, 0x69, 0x6E, 0x67, 0x20, 0x33, 0x20,
    0x61, 0x74, 0x74, 0x65, 0x6D, 0x70, 0x74, 0x69, 0x6E, 0x67, 0x20, 0x43,
    0x4C, 0x49, 0x20, 0x69, 0x6E, 0x73, 0x74, 0x72, 0x75, 0x63, 0x74, 0x69,
    0x6F, 0x6E, 0x2E, 0x2E, 0x2E, 0x0A, 0x00
};*/


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

    // 가상 파일 시스템 초기화
    print_string("Initializing VFS...");
    vfs_init();
    print_string(" Done.\n");

    // 멀티태스킹 초기화
    print_string("Initializing Task...");
    init_tasking();
    create_task(spinner);
    print_string(" Done.\n");

    /*
    vfs_node check_node;
    if (vfs_resolve_path("/hello.bin", &check_node) != 0) {
        vfs_node hello_file;
        if (vfs_create(&vfs_root, "hello.bin", &hello_file) == 0) {
            vfs_write(&hello_file, sample_hello_bin, sizeof(sample_hello_bin));
        }
    }
    // 디스크에 /bad.bin 생성
    if (vfs_resolve_path("/bad.bin", &check_node) != 0) {
        vfs_node bad_file;
        if (vfs_create(&vfs_root, "bad.bin", &bad_file) == 0) {
            vfs_write(&bad_file, sample_bad_bin, sizeof(sample_bad_bin));
        }
    }*/



    // CPU 인터럽트 활성화
    __asm__ __volatile__ ("sti");
    print_string("Interrupts enabled.\n\n");

    // 쉘 시작
    shell_main();

}
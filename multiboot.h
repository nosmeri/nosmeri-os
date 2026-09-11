#pragma once

#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

// Multiboot 정보 플래그
#define MULTIBOOT_INFO_MEMORY      0x00000001
#define MULTIBOOT_INFO_BOOTDEV     0x00000002
#define MULTIBOOT_INFO_CMDLINE     0x00000004
#define MULTIBOOT_INFO_MODS        0x00000008
#define MULTIBOOT_INFO_AOUT_SYMS   0x00000010
#define MULTIBOOT_INFO_ELF_SHDR    0x00000020
#define MULTIBOOT_INFO_MEM_MAP     0x00000040

// E820 메모리 타입
#define MULTIBOOT_MEMORY_AVAILABLE        1
#define MULTIBOOT_MEMORY_RESERVED         2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE 3
#define MULTIBOOT_MEMORY_NVS              4
#define MULTIBOOT_MEMORY_BADRAM           5

// E820 메모리 맵 엔트리 구조체
struct multiboot_mmap_entry {
    unsigned int size;
    unsigned int addr_low;
    unsigned int addr_high;
    unsigned int len_low;
    unsigned int len_high;
    unsigned int type;
} __attribute__((packed));

// Multiboot 1 스펙 기본 정보 구조체
struct multiboot_info {
    unsigned int flags;
    unsigned int mem_lower; // KB 단위 (보통 640KB)
    unsigned int mem_upper; // KB 단위 (1MB 이상 사용 가능한 RAM 크기)
    unsigned int boot_device;
    unsigned int cmdline;
    unsigned int mods_count;
    unsigned int mods_addr;
    unsigned int num;
    unsigned int size;
    unsigned int addr;
    unsigned int shndx;
    unsigned int mmap_length;
    unsigned int mmap_addr;
} __attribute__((packed));

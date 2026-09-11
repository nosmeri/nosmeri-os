#pragma once

#define PAGE_PRESENT  0x1 // Bit 0: 메모리에 존재함
#define PAGE_RW       0x2 // Bit 1: 읽기/쓰기 가능 (0이면 읽기 전용)
#define PAGE_USER     0x4 // Bit 2: 유저 모드 접근 가능 (0이면 커널 전용)

void init_vmm();
void vmm_map_page(unsigned int virt_addr, unsigned int phys_addr, unsigned int flags);
void vmm_unmap_page(unsigned int virt_addr);

// 14번 Page Fault 예외 핸들러 (boot.asm의 isr14에서 호출)
extern "C" void page_fault_handler(unsigned int error_code);

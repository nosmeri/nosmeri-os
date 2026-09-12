#include "pmm.h"
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
    // 재귀 접근을 위한 매핑
    page_directory[1023] = (unsigned int) page_directory | PAGE_PRESENT | PAGE_RW;

    // 초기 테이블 물리주소와 1:1 매핑
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

// 페이지 매핑 함수
void vmm_map_page(unsigned int virt_addr, unsigned int phys_addr, unsigned int flags) {
    unsigned int pd_idx = virt_addr >> 22;
    unsigned int pt_idx = (virt_addr >> 12) & 0x3FF;

    // 재귀 페이징을 이용한 페이지 테이블 접근
    unsigned int* page_table = (unsigned int*)(0xFFC00000 | (pd_idx << 12));

    // 필요한 페이지 테이블이 없다면 할당
    if (!(page_directory[pd_idx] & PAGE_PRESENT)) {
        void* new_pt_phys = pmm_alloc_page();

        page_directory[pd_idx] = (unsigned int)new_pt_phys | PAGE_PRESENT | PAGE_RW | PAGE_USER;

        // TLB 갱신 (재귀 페이징을 사용하므로 직접 페이지 테이블 주소를 인자로 넘김)
        asm volatile("invlpg (%0)" : : "r"(page_table) : "memory");

        // 새로 만든 페이지 테이블 초기화
        for (int i = 0; i < 1024; i++) {
            page_table[i]=0;
        }
    }

    // 페이지 테이블에 실제 물리 주소와 플래그 설정
    page_table[pt_idx] = (phys_addr & ~0xFFF) | flags;

    // CPU TLB 캐시 갱신
    asm volatile("invlpg (%0)" : : "r"(virt_addr) : "memory");
}

void vmm_unmap_page(unsigned int virt_addr) {
    unsigned int pd_idx = virt_addr >> 22;
    unsigned int pt_idx = (virt_addr >> 12) & 0x3FF;

    if (!(page_directory[pd_idx] & PAGE_PRESENT)) return;

    unsigned int* page_table = (unsigned int*)(0xFFC00000 | (pd_idx << 12));

    page_table[pt_idx] = 0;

    asm volatile("invlpg (%0)" : : "r"(virt_addr) : "memory");
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
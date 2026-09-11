#include "pmm.h"
#include "vga.h"

// linker.ld에 정의된 커널 끝 심볼
extern "C" unsigned int kernel_end;

// 최대 4GB 물리 메모리를 관리하기 위한 비트맵 크기
// 4GB / 4KB = 1,048,576 개 페이지. 1개 uint(32비트)는 32개 페이지를 관리하므로 32,768개 필요 (128KB 크기)
#define MAX_PAGES (1024 * 1024)
#define BITMAP_SIZE (MAX_PAGES / 32)

static unsigned int memory_bitmap[BITMAP_SIZE];

static unsigned int total_memory_pages = 0;
static unsigned int used_memory_pages  = 0;

// 비트 연산 헬퍼: n번째 비트를 1로 설정 (페이지 사용 중 표시)
static inline void bitmap_set(unsigned int page_idx) {
    if (page_idx < total_memory_pages) {
        if (!(memory_bitmap[page_idx / 32] & (1 << (page_idx % 32)))) {
            memory_bitmap[page_idx / 32] |= (1 << (page_idx % 32));
            used_memory_pages++;
        }
    }
}

// 비트 연산 헬퍼: n번째 비트를 0으로 설정 (페이지 비어있음 표시)
static inline void bitmap_unset(unsigned int page_idx) {
    if (page_idx < total_memory_pages) {
        if (memory_bitmap[page_idx / 32] & (1 << (page_idx % 32))) {
            memory_bitmap[page_idx / 32] &= ~(1 << (page_idx % 32));
            used_memory_pages--;
        }
    }
}

// 비트 연산 헬퍼: n번째 비트 상태 확인 (true = 사용 중, false = 비어있음)
static inline bool bitmap_test(unsigned int page_idx) {
    return memory_bitmap[page_idx / 32] & (1 << (page_idx % 32));
}

// 첫 번째 빈 비트(0) 찾기
static int find_first_free_page() {
    for (unsigned int i = 0; i < total_memory_pages / 32; i++) {
        if (memory_bitmap[i] != 0xFFFFFFFF) { // 32개 비트 중 적어도 하나가 0임
            for (int j = 0; j < 32; j++) {
                unsigned int page_idx = i * 32 + j;
                if (page_idx >= total_memory_pages) return -1;
                if (!(memory_bitmap[i] & (1 << j))) {
                    return page_idx;
                }
            }
        }
    }
    return -1;
}

// 특정 주소 범위 [base, base + length)를 사용 중(1)으로 마킹
static void pmm_mark_region_used(unsigned int base, unsigned int length) {
    unsigned int start_page = base / PAGE_SIZE;
    unsigned int num_pages  = (length + PAGE_SIZE - 1) / PAGE_SIZE;

    for (unsigned int i = 0; i < num_pages; i++) {
        bitmap_set(start_page + i);
    }
}

// 특정 주소 범위 [base, base + length)를 사용 가능(0)으로 마킹
static void pmm_mark_region_free(unsigned int base, unsigned int length) {
    unsigned int start_page = (base + PAGE_SIZE - 1) / PAGE_SIZE;
    unsigned int end_page   = (base + length) / PAGE_SIZE;

    for (unsigned int i = start_page; i < end_page; i++) {
        bitmap_unset(i);
    }
}

// PMM 초기화
void init_pmm(unsigned int magic, multiboot_info* mbi) {
    unsigned int memory_size_kb = 0;

    // 1. 기본값: 128MB (QEMU 기본 부팅)
    if (magic == MULTIBOOT_BOOTLOADER_MAGIC && (mbi->flags & MULTIBOOT_INFO_MEMORY)) {
        // mem_lower는 1MB 이하(보통 640KB), mem_upper는 1MB 이상의 크기(KB)
        memory_size_kb = 1024 + mbi->mem_upper;
    } else {
        memory_size_kb = 128 * 1024; // 128MB 폴백
    }

    total_memory_pages = (memory_size_kb * 1024) / PAGE_SIZE;
    if (total_memory_pages > MAX_PAGES) {
        total_memory_pages = MAX_PAGES;
    }

    // 2. 일단 모든 메모리를 1(사용 중)로 잠금
    for (unsigned int i = 0; i < BITMAP_SIZE; i++) {
        memory_bitmap[i] = 0xFFFFFFFF;
    }
    used_memory_pages = total_memory_pages;

    // 3. E820 메모리 맵이 제공되면 사용 가능한(Type=1) 영역만 0(Free)으로 열어줌
    if (magic == MULTIBOOT_BOOTLOADER_MAGIC && (mbi->flags & MULTIBOOT_INFO_MEM_MAP)) {
        multiboot_mmap_entry* mmap = (multiboot_mmap_entry*)mbi->mmap_addr;
        unsigned int mmap_end = mbi->mmap_addr + mbi->mmap_length;

        while ((unsigned int)mmap < mmap_end) {
            if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
                pmm_mark_region_free(mmap->addr_low, mmap->len_low);
            }
            mmap = (multiboot_mmap_entry*)((unsigned int)mmap + mmap->size + sizeof(mmap->size));
        }
    } else {
        // E820 맵이 없으면 1MB 이상 전체를 사용 가능으로 설정
        pmm_mark_region_free(0x100000, (memory_size_kb - 1024) * 1024);
    }

    // 4. 필수 보호 영역 잠금 (다시 1로 설정):
    // - 0x00000 ~ 0x100000 (0 ~ 1MB: BIOS IVT, BDA, VGA 버퍼 0xB8000, ROM)
    pmm_mark_region_used(0x0, 0x100000);

    // - 1MB ~ 커널 코드 끝(kernel_end) 영역
    unsigned int kernel_end_addr = (unsigned int)&kernel_end;
    pmm_mark_region_used(0x100000, kernel_end_addr - 0x100000);
}

// 4KB 물리 페이지 할당
void* pmm_alloc_page() {
    int free_page = find_first_free_page();
    if (free_page == -1) {
        return 0; // Out of memory!
    }

    bitmap_set(free_page);
    return (void*)(free_page * PAGE_SIZE);
}

// 4KB 물리 페이지 반납
void pmm_free_page(void* ptr) {
    unsigned int addr = (unsigned int)ptr;
    unsigned int page_idx = addr / PAGE_SIZE;

    bitmap_unset(page_idx);
}

// 통계 정보
unsigned int pmm_get_total_pages() {
    return total_memory_pages;
}

unsigned int pmm_get_used_pages() {
    return used_memory_pages;
}

unsigned int pmm_get_free_pages() {
    return total_memory_pages - used_memory_pages;
}

unsigned int pmm_get_total_memory_kb() {
    return (total_memory_pages * PAGE_SIZE) / 1024;
}

unsigned int pmm_get_used_memory_kb() {
    return (used_memory_pages * PAGE_SIZE) / 1024;
}

unsigned int pmm_get_free_memory_kb() {
    return (pmm_get_free_pages() * PAGE_SIZE) / 1024;
}

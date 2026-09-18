#include "heap.h"
#include "pmm.h"
#include "vmm.h"
#include "vga.h"
#include "string.h"

static struct block_header* heap_head;
static unsigned int heap_current_end;

// 힙 메모리 초기화
void init_heap() {
    // 초기 힙 메모리의 물리 메모리 할당 및 페이지 할당
    for (unsigned int addr = HEAP_START; addr < HEAP_START + HEAP_INITIAL_SIZE; addr += PAGE_SIZE){
        void* heap_phys_addr = pmm_alloc_page();
        vmm_map_page(addr, (unsigned int)heap_phys_addr, PAGE_PRESENT | PAGE_RW | PAGE_USER);
    }

    // 초기 block header 설정
    struct block_header* block = (struct block_header*) HEAP_START;
    block->size = HEAP_INITIAL_SIZE-sizeof(block_header);
    block->is_free = true;
    block->next = 0;
    block->prev = 0;
    heap_head = block;
    heap_current_end = HEAP_START + HEAP_INITIAL_SIZE;
}

// 힙 메모리 할당
void* kmalloc(unsigned int size) {
    if (size == 0) return 0;
    // 4B로 정렬
    unsigned int aligned_size = ((size+3)/4)*4;
    struct block_header* block = heap_head;
    struct block_header* prev_block = 0;

    // 해제된 블록 중에서 요청한 size보다 크고 적절한 블록 찾기
    while(block!=0){
        if(block->is_free && block->size>=aligned_size){
            if (block->size - aligned_size >= sizeof(block_header)+4) { //split block
                struct block_header* new_block = (struct block_header*)((unsigned int)block + aligned_size + sizeof(block_header));
                new_block->size = block->size - aligned_size - sizeof(block_header);
                new_block->is_free = true;
                new_block->next = block->next;
                new_block->prev = block;
                if (block->next) {
                    block->next->prev = new_block;
                }
                block->next = new_block;
                block->size = aligned_size;
            }
            block->is_free = false;
            return (void*)((unsigned int)block + sizeof(block_header));
        }
        prev_block=block;
        block = block->next;
    }
    
    // 적절한 block을 찾지 못하면 새로운 페이지 할당

    // 힙의 가상 주소 상한선 검사
    if (heap_current_end + PAGE_SIZE > HEAP_MAX_ADDR) {
        return 0; // 더 이상 가상 주소를 확장할 수 없음 (NULL 반환)
    }
    // 물리 메모리 할당
    void* heap_phys_addr = pmm_alloc_page();
    if (!heap_phys_addr) {
        return 0; // 실제 RAM이 바닥남
    }
    
    vmm_map_page(heap_current_end, (unsigned int)heap_phys_addr, PAGE_PRESENT | PAGE_RW | PAGE_USER);
    if (prev_block && prev_block->is_free) {
        // 직전 블록이 비어있다면, 헤더를 새로 만들 필요 없이 기존 블록 크기만 4KB 늘려줌
        prev_block->size += PAGE_SIZE;
        heap_current_end += PAGE_SIZE;
        return kmalloc(size); // 이제 합쳐져서 커졌으니 재귀 호출 시 바로 할당 성공
    } else {
        struct block_header* new_block = (struct block_header*) heap_current_end;
        new_block->size = PAGE_SIZE-sizeof(block_header);
        new_block->is_free = true;
        new_block->next = 0;
        new_block->prev = prev_block;
        prev_block->next = new_block;
        heap_current_end += PAGE_SIZE;
        return kmalloc(size);
    }
}

// 힙 메모리 해제
void kfree(void* ptr) {
    if (!ptr) return;
    block_header* block = (block_header*)ptr - 1;
    block->is_free = true;

    // 옆의 블록과 합치기
    if (block->next && block->next->is_free) {
        block->size += block->next->size + sizeof(block_header);
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }
    if (block->prev && block->prev->is_free) {
        block->prev->size += block->size + sizeof(block_header);
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }

    }
}

// 힙 메모리의 상태를 시각적으로 확인하기 위한 함수
void heap_dump() {
    block_header* curr = heap_head;
    char buff[32];

    print_string("=== KERNEL HEAP DUMP ===\n");

    unsigned int block_cnt = 0;
    unsigned int used_cnt = 0;
    unsigned int free_cnt = 0;

    while (curr != 0) {
        print_string("[#");
        itoa(block_cnt, buff, 10);
        print_string(buff);
        print_string("] Addr: 0x");
        
        itoa((unsigned int)curr, buff, 16);
        print_string(buff);

        print_string(" | Size:");
        
        itoa(curr->size, buff, 10);
        print_string(buff);
        
        print_string(" | Status: ");
        print_string(curr->is_free ? "[FREE]" : "[USED]");
        print_char('\n');
        
        
        block_cnt++;
        if (curr->is_free) free_cnt++;
        else used_cnt++;

        curr = curr->next;
    }

    print_string("--------------------------------------------------\nTotal Blocks: ");

    itoa(block_cnt, buff, 10);
    print_string(buff);
    print_string(" (Used: ");

    itoa(used_cnt, buff, 10);
    print_string(buff);
    print_string(", Free: ");

    itoa(free_cnt, buff, 10);
    print_string(buff);
    print_string(")\n");
}
#include "heap.h"
#include "pmm.h"
#include "vmm.h"

static struct block_header* heap_head;
static unsigned int heap_current_end;

void init_heap() {
    for (unsigned int addr = HEAP_START; addr < HEAP_START + HEAP_INITIAL_SIZE; addr += PAGE_SIZE){
        void* heap_phys_addr = pmm_alloc_page();
        vmm_map_page(addr, (unsigned int)heap_phys_addr, PAGE_PRESENT | PAGE_RW);
    }

    struct block_header* block = (struct block_header*) HEAP_START;
    block->size = HEAP_INITIAL_SIZE-sizeof(block_header);
    block->is_free = true;
    block->next = 0;
    block->prev = 0;
    heap_head = block;
    heap_current_end = HEAP_START + HEAP_INITIAL_SIZE;
}

void* kmalloc(unsigned int size) {
    if (size == 0) return 0;
    unsigned int aligned_size = ((size+3)/4)*4;
    struct block_header* block = heap_head;
    struct block_header* prev_block = 0;
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

    void* heap_phys_addr = pmm_alloc_page();
    vmm_map_page(heap_current_end, (unsigned int)heap_phys_addr, PAGE_PRESENT | PAGE_RW);
    struct block_header* new_block = (struct block_header*) heap_current_end;
    new_block->size = PAGE_SIZE-sizeof(block_header);
    new_block->is_free = true;
    new_block->next = 0;
    new_block->prev = prev_block;
    prev_block->next = new_block;
    heap_current_end += PAGE_SIZE;
    return kmalloc(size);
}

void kfree(void* ptr) {
    if (!ptr) return;
    block_header* block = (block_header*)ptr - 1;
    block->is_free = true;
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
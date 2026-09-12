#pragma once

// 힙 메모리의 가상 주소
#define HEAP_START 0xC0000000
#define HEAP_INITIAL_SIZE 0x1000 
#define HEAP_MAX_ADDR 0xE0000000

struct block_header {
    unsigned int size;
    bool is_free;
    struct block_header* next;
    struct block_header* prev;
};

void init_heap();
void* kmalloc(unsigned int size);
void kfree(void* ptr);
void heap_dump();
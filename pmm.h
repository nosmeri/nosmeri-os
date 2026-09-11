#pragma once

#include "multiboot.h"

#define PAGE_SIZE 4096 // 4KB

// PMM 초기화 함수
void init_pmm(unsigned int magic, multiboot_info* mbi);

// 4KB 물리 페이지 프레임 할당 및 해제
void* pmm_alloc_page();
void pmm_free_page(void* ptr);

// 메모리 정보 통계 함수 (KB 단위 및 페이지 수 단위)
unsigned int pmm_get_total_pages();
unsigned int pmm_get_used_pages();
unsigned int pmm_get_free_pages();
unsigned int pmm_get_total_memory_kb();
unsigned int pmm_get_used_memory_kb();
unsigned int pmm_get_free_memory_kb();

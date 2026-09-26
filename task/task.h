#pragma once
#include "pmm.h"
#include "file.h"

#define MAX_FD 16
#define TASK_STACK_SIZE 1024
#define USER_STACK_TOP    0xBFFFF000
#define USER_STACK_BOTTOM (USER_STACK_TOP - PAGE_SIZE) // 4KB 크기
#define USER_CODE_START   0x40000000

enum TaskState {
    TASK_READY,
    TASK_RUNNING,
    TASK_SLEEPING,
    TASK_DEAD
};
struct Task {
    unsigned int id;              // 태스크 고유 ID (PID)
    unsigned int esp;             // 이 태스크가 멈췄을 때의 스택 포인터 (ESP)
    void* kernel_stack_bottom;    // kmalloc으로 할당받은 커널 스택 메모리 주소 (해제용)
    void* user_stack_bottom;      // 유저 스택 할당 주소 (태스크 종료 시 kfree용)
    unsigned int kernel_stack_top;// 커널 스택 최상단 주소
    unsigned int cr3;
    bool is_user;                 // 유저 태스크인지 여부 (true/false)
    TaskState state;              // 현재 상태
    unsigned int wake_tick;
    Task* next;                   // 원형 연결 리스트(Circular Linked List)용 포인터
    File* fd_table[MAX_FD];
    vfs_node cwd;
};

void task_sleep(unsigned int ms);
void task_timer_tick();
void init_tasking();
Task* get_current_task();
Task* create_task(void (*entry_point)());
Task* create_user_task(void (*entry_point)());
Task* create_user_process(const char* filepath);
void task_yield();
void schedule();
int kill_task(unsigned int pid);
void exit_task();
void task_dump();
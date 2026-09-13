#pragma once

#define TASK_STACK_SIZE 1024

enum TaskState {
    TASK_READY,
    TASK_RUNNING,
    TASK_SLEEPING,
    TASK_DEAD
};
struct Task {
    unsigned int id;              // 태스크 고유 ID (PID)
    unsigned int esp;             // 이 태스크가 멈췄을 때의 스택 포인터 (ESP)
    void* stack_bottom;           // kmalloc으로 할당받은 스택 메모리 주소 (해제용)
    TaskState state;              // 현재 상태
    Task* next;                   // 원형 연결 리스트(Circular Linked List)용 포인터
};

void init_tasking();
Task* create_task(void (*entry_point)());
void task_yield();
void schedule();
void task_dump();

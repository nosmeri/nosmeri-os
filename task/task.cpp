#include "task.h"
#include "heap.h"
#include "vga.h"
#include "string.h"
#include "timer.h"

static Task kernel_task;
static Task* current_task = 0;
static unsigned int next_pid = 0;

// 커널 프로세스 -> 인터럽트 1 -> switch -> 프로세스 2 -> 인터럽트 2 -> switch -> 인터럽트 1 -> 커널프로세스 

// 프로세스 생성 (fork 원리)
Task* create_task(void (*entry_point)()) {
    // 태스크 스택(4KB) 할당
    void* stack_mem = kmalloc(TASK_STACK_SIZE);
    if (!stack_mem) return 0;

    Task* new_task = (Task*)kmalloc(sizeof(Task));
    if (!new_task) {
        kfree(stack_mem);
        return 0;
    }

    // 스택 초기화
    unsigned int* stack_top = (unsigned int*)((unsigned int)stack_mem + TASK_STACK_SIZE);

    // ret가 점프할 함수 주소
    stack_top[-1] = (unsigned int)entry_point;
    // popfd가 복원할 EFLAGS (0x202 = 인터럽트 허용)
    stack_top[-2] = 0x0202;
    // popa가 복원할 8개 레지스터 (EAX, ECX, EDX, EBX, 더미ESP, EBP, ESI, EDI)
    // ESP는 popa 할때 
    for (int i = 3; i <= 10; i++) {
        stack_top[-i] = 0;
    }
    // 이 태스크의 시작 ESP는 EDI가 있는 위치(&stack_top[-10])
    // context switch용 esp
    new_task->esp = (unsigned int)&stack_top[-10];
    new_task->stack_bottom = stack_mem;
    new_task->id = next_pid++;
    new_task->state = TASK_READY;
    new_task->wake_tick = 0;

    // 원형 연결 리스트에 추가
    new_task->next = current_task->next;
    current_task->next = new_task;

    return new_task;
}

extern "C" void switch_context(Task* prev, Task* next);

void init_tasking() {
    kernel_task.id = next_pid++;
    kernel_task.esp = 0;          
    kernel_task.stack_bottom = 0;    
    kernel_task.state = TASK_RUNNING;
    kernel_task.wake_tick = 0;
    kernel_task.next = &kernel_task; 
    current_task = &kernel_task;
}

void task_sleep(unsigned int ms) {
    if (!current_task) return; // 현재 태스크 없으면 반환

    unsigned int ticks = (ms * TIMER_FREQ) / 1000;
    if (ms > 0 && ticks == 0) ticks = 1;

    current_task->wake_tick = get_tick() + ticks;
    current_task->state = TASK_SLEEPING;

    schedule();
}

// 원형 리스트를 돌며 슬립 상태인 태스크 체크
void task_timer_tick() {
    if (!current_task) return; // 현재 태스크 없으면 반환

    unsigned int current_tick = get_tick();
    Task* t = current_task;

    do {
        if (t->state == TASK_SLEEPING) {
            if (current_tick >= t->wake_tick) {
                t->state = TASK_READY; // 시간이 다 됐으니 다시 실행 준비 완료!
            }
        }
        t = t->next;
    } while (t != current_task);
}

void task_yield() {
    schedule();
}

void schedule() {
    // 태스크가 없거나 1개면 스케쥴링 X
    if (!current_task || current_task->next == current_task) return;
    
    Task* prev = current_task;
    Task* next = current_task->next;

    while (next->state != TASK_READY && next != prev) {
        next = next->next;
    }

    // 모든 태스크가 실행 준비 안되면 스위칭 X
    if (next->state != TASK_READY) {
        return;
    }

    if (prev->state == TASK_RUNNING) {
        prev->state = TASK_READY;
    }

    next->state = TASK_RUNNING;

    current_task = next;
    switch_context(prev, next); 
}

int kill_task(unsigned int pid) {
    if (!current_task) return -1;
    if (pid == 0) {
        return -1;
    }

    Task* t = current_task->next;
    Task* prev = current_task;
    do {
        if (t->id == pid) {
            prev->next = t->next;
            kfree(t->stack_bottom);
            kfree(t);

            return pid;
        }
        prev = t;
        t = t->next;
    } while (t != current_task);

    return -1;
}

void task_dump() {
    if (!current_task) return;
    print_string("PID   STATE     ESP         STACK_ADDR\n");
    print_string("-----------------------------------------\n");
    Task* t = current_task;
    char buf[32];
    do {
        print_string(" ");
        itoa(t->id, buf, 10);
        print_string(buf);
        print_string("     ");
        switch (t->state) {
            case TASK_RUNNING:
                print_string("RUNNING   ");
                break;
            case TASK_READY:
                print_string("READY     ");
                break;
            case TASK_SLEEPING:
                print_string("SLEEPING  ");
                break;
            default:
                print_string("DEAD      ");
                break;
        }
        print_string("0x");
        itoa(t->esp, buf, 16);
        print_string(buf);
        print_string("  ");
        print_string("0x");
        itoa((unsigned int)t->stack_bottom, buf, 16);
        print_string(buf);
        print_string("\n");
        t = t->next;
    } while (t != current_task);
}
#include "task.h"
#include "heap.h"
#include "vga.h"
#include "string.h"
#include "timer.h"
#include "gdt.h"
#include "vmm.h"
#include "pmm.h"
#include "vfs.h"
#include "file.h"
#include "syscall.h"

static Task kernel_task;
static Task* current_task = 0;
static unsigned int next_pid = 0;

// 커널 프로세스 -> 인터럽트 1 -> switch -> 프로세스 2 -> 인터럽트 2 -> switch -> 인터럽트 1 -> 커널프로세스 

void init_task_fds(Task* t);

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
    
    stack_top[-1] = (unsigned int)exit_task;
    // ret가 점프할 함수 주소
    stack_top[-2] = (unsigned int)entry_point;
    // popfd가 복원할 EFLAGS (0x202 = 인터럽트 허용)
    stack_top[-3] = 0x0202;
    // popa가 복원할 8개 레지스터 (EAX, ECX, EDX, EBX, 더미ESP, EBP, ESI, EDI)
    // ESP는 popa 할때 
    for (int i = 4; i <= 11; i++) {
        stack_top[-i] = 0;
    }
    // 이 태스크의 시작 ESP는 EDI가 있는 위치(&stack_top[-11])
    // context switch용 esp
    new_task->esp = (unsigned int)&stack_top[-11];
    new_task->kernel_stack_bottom = stack_mem;
    new_task->kernel_stack_top = (unsigned int)stack_mem + TASK_STACK_SIZE;
    new_task->user_stack_bottom = 0;
    new_task->cr3 = kernel_task.cr3;
    new_task->is_user = false; 
    new_task->id = next_pid++;
    new_task->state = TASK_READY;
    new_task->wake_tick = 0;
    init_task_fds(new_task);
    new_task->cwd = current_task ? current_task->cwd : vfs_root;

    // 원형 연결 리스트에 추가
    new_task->next = current_task->next;
    current_task->next = new_task;

    return new_task;
}

extern "C" void user_task_trampoline();

Task* create_user_task(void (*entry_point)()) {
    // 커널 스택(4KB) 할당
    void* kernel_stack_mem = kmalloc(TASK_STACK_SIZE);
    // 유저 스택(4KB) 할당
    void* user_stack_mem = pmm_alloc_page();

    if (!kernel_stack_mem || !user_stack_mem) {
        if (kernel_stack_mem) kfree(kernel_stack_mem);
        if (user_stack_mem) pmm_free_page(user_stack_mem);
        return 0;
    }
    
    Task* new_task = (Task*)kmalloc(sizeof(Task));
    if (!new_task) {
        kfree(kernel_stack_mem);
        pmm_free_page(user_stack_mem);
        return 0;
    }

    new_task->cr3 = vmm_create_page_directory();
    vmm_switch_page_directory(new_task->cr3);

    vmm_map_page(USER_STACK_BOTTOM, (unsigned int)user_stack_mem, PAGE_USER | PAGE_RW | PAGE_PRESENT);
    
    // 스택 초기화
    unsigned int* kernel_stack_top = (unsigned int*)((unsigned int)kernel_stack_mem + TASK_STACK_SIZE);

    vmm_switch_page_directory(current_task->cr3);
    
    // 커널 스택에 저장할 유저 태스크 복귀 초기 정보
    // 이후에는 인터럽트시 자동으로 저장됨
    // iret 프레임 (user_task_trampoline의 iret이 꺼내먹을 5개)
    kernel_stack_top[-1] = 0x23;                      // User SS
    kernel_stack_top[-2] = USER_STACK_TOP;            // User ESP
    kernel_stack_top[-3] = 0x0202;                    // EFLAGS (IF=1 인터럽트 허용)
    kernel_stack_top[-4] = 0x1B;                      // User CS (0x18 | 3)
    kernel_stack_top[-5] = (unsigned int)entry_point; // User EIP

    // switch_context 프레임 (switch_context가 꺼내먹을 부분)
    kernel_stack_top[-6] = (unsigned int)user_task_trampoline; // ret이 점프할 주소
    kernel_stack_top[-7] = 0x0202;                    // popfd가 복원할 EFLAGS
    
    // popa가 복원할 8개 레지스터 (8 ~ 15번)
    for (int i = 8; i <= 15; i++) {
        kernel_stack_top[-i] = 0;
    }

    // switch_context가 읽을 시작 ESP는 EDI 위치(-15번)
    new_task->esp = (unsigned int)&kernel_stack_top[-15];

    new_task->kernel_stack_bottom = kernel_stack_mem; // 가상메모리
    new_task->kernel_stack_top = (unsigned int)kernel_stack_mem + TASK_STACK_SIZE;
    new_task->user_stack_bottom = user_stack_mem; // 물리메모리
    new_task->is_user = true; 
    new_task->id = next_pid++;
    new_task->state = TASK_READY;
    new_task->wake_tick = 0;
    init_task_fds(new_task);
    new_task->cwd = current_task ? current_task->cwd : vfs_root;

    // 원형 연결 리스트에 추가
    new_task->next = current_task->next;
    current_task->next = new_task;

    return new_task;
}

// 유저 프로세스 생성(파일 읽어서 실행)
Task* create_user_process(const char* filepath) {
    // 파일 읽기
    vfs_node file_node;
    if (vfs_resolve_path(filepath, &file_node) != 0) {
        return 0;
    }
    
    unsigned char* buf = (unsigned char*)kmalloc(file_node.size);
    if (!buf) return 0;

    vfs_read(&file_node, buf, file_node.size);

    Task* new_task = create_user_task((void(*)())USER_CODE_START);

    if(!new_task) {
        kfree(buf);
        return 0;
    }
    new_task->state = TASK_SLEEPING;
    new_task->wake_tick = 0xFFFFFFFF;
    
    vmm_switch_page_directory(new_task->cr3);

    unsigned int page_cnt = (file_node.size + PAGE_SIZE - 1) / PAGE_SIZE;

    for (unsigned int i = 0; i < page_cnt; i++) {
        void* frame = pmm_alloc_page();
        vmm_map_page(USER_CODE_START + i*PAGE_SIZE, (unsigned int)frame, PAGE_USER | PAGE_RW | PAGE_PRESENT);
    }

    // 파일 내용을 유저 코드 영역에 복사
    memcpy((void*)USER_CODE_START, buf, file_node.size);
    kfree(buf);

    vmm_switch_page_directory(current_task->cr3);

    new_task->state = TASK_READY;
    new_task->wake_tick = 0;

    return new_task;
}

Task* get_current_task() { return current_task; }

extern "C" void switch_context(Task* prev, Task* next);

void init_tasking() {
    kernel_task.id = next_pid++;
    kernel_task.esp = 0;          
    kernel_task.kernel_stack_bottom = 0;    
    kernel_task.cr3 = vmm_get_kernel_page_directory();
    kernel_task.is_user = false;
    kernel_task.state = TASK_RUNNING;
    kernel_task.wake_tick = 0;
    init_task_fds(&kernel_task);
    kernel_task.next = &kernel_task; 
    // 커널 태스크의 현재 작업 디렉터리를 루트로 설정
    vfs_resolve_path("/", &kernel_task.cwd);
    current_task = &kernel_task;
}

void init_task_fds(Task* t) {
    // 1. 모든 슬롯 비우기
    for (int i = 0; i < MAX_FD; i++) {
        t->fd_table[i] = 0;
    }
    // 2. 0, 1, 2 기본 할당
    t->fd_table[0] = create_stdin_file();
    t->fd_table[1] = create_stdout_file();
    t->fd_table[2] = create_stderr_file();
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
        if(next->state == TASK_DEAD) {
            Task* temp = next->next;
            kill_task(next->id);
            next = temp;
            continue;
        }
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

    if (next->is_user) {
        // 유저 태스크 -> iret이 복원할 위치(User ESP)를 가리킴
        set_kernel_stack((unsigned int)next->kernel_stack_top);
    }

    // 페이지 디렉토리(CR3) 변경
    if (prev->cr3 != next->cr3) {
        vmm_switch_page_directory(next->cr3);
    }

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
            kfree(t->kernel_stack_bottom);
            if (t->is_user) {
                pmm_free_page(t->user_stack_bottom);
                pmm_free_page((void*)t->cr3);
            }
            kfree(t);

            return pid;
        }
        prev = t;
        t = t->next;
    } while (t != current_task);

    return -1;
}

void exit_task() {
    current_task->state = TASK_DEAD;
    schedule();

    while (true) {
        __asm__ __volatile__ ("hlt");
    }
}

void task_dump() {
    if (!current_task) return;
    print_string("PID   MODE    STATE     ESP         STACK_ADDR\n");
    print_string("----------------------------------------------------\n");
    Task* t = current_task;
    char buf[32];
    do {
        print_string(" ");
        itoa(t->id, buf, 10);
        print_string(buf);
        print_string("     ");

        if (t->is_user) {
            print_string("USER    ");
        } else {
            print_string("KERNEL  ");
        }

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
        itoa((unsigned int)t->kernel_stack_bottom, buf, 16);
        print_string(buf);
        print_string("\n");
        t = t->next;
    } while (t != current_task);
}
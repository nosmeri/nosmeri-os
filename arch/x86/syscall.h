#pragma once

// 스택에 push된 레지스터 매핑 구조체
// boot.asm의 isr80에서 push한 역순으로 정의됩니다.
struct Registers {
    // 세그먼트 레지스터 (수동 push)
    unsigned int gs, fs, es, ds;
    // 범용 레지스터 (pusha로 push: EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX 순)
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;
    // CPU가 인터럽트 발생 시 자동으로 스택에 넣는 정보
    unsigned int eip, cs, eflags;
} __attribute__((packed));

// 시스템 콜 번호 정의
#define SYS_GETTICK 1
#define SYS_EXIT 2
#define SYS_SLEEP 3
#define SYS_READ  4
#define SYS_WRITE 5
#define SYS_OPEN  6
#define SYS_CLOSE 7

// C++ 시스템 콜 핸들러 (boot.asm의 isr80에서 호출)
extern "C" void syscall_handler(Registers* regs);

// 호출자 편의를 위한 래퍼 함수 (소프트웨어 인터럽트 int 0x80 발생)
unsigned int sys_get_tick();
void sys_exit();
void sys_sleep(unsigned int delay_ticks);
int sys_write(int fd, const void* buf, unsigned int count);
int sys_read(int fd, void* buf, unsigned int count);
#include "syscall.h"
#include "vga.h"
#include "timer.h"

// C++ 시스템 콜 핸들러 본체
// boot.asm의 isr80에서 push esp 한 포인터가 regs로 전달됨
extern "C" void syscall_handler(Registers* regs) {
    switch (regs->eax) {
        case SYS_PRINT: {
            // ebx 레지스터에 문자열의 포인터가 담겨 전달됨
            const char* str = (const char*)regs->ebx;
            if (str != 0) {
                print_string(str);
            }
            break;
        }

        case SYS_GETTICK: {
            // 반환값은 eax 레지스터에 저장
            regs->eax = get_tick();
            break;
        }

        default:
            // 알 수 없는 시스템 콜 번호
            break;
    }
}

// 1번 시스템 콜(SYS_PRINT)을 호출하는 함수
void sys_print(const char* str) {
    __asm__ __volatile__ (
        "int $0x80"
        :
        : "a"(SYS_PRINT), "b"(str)
        : "memory"
    );
}

// 2번 시스템 콜(SYS_GETTICK)을 호출하는 함수
unsigned int sys_get_tick() {
    unsigned int ret;
    __asm__ __volatile__ (
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_GETTICK)
        : "memory"
    );
    return ret;
}
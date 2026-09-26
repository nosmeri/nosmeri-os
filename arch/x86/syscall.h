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

// 1 ~ 9: 프로세스 및 태스크 제어
#define SYS_EXIT       1   // 프로세스 종료 (리눅스도 1번)
#define SYS_SLEEP      2   // 태스크 대기 (ms)
#define SYS_YIELD      3   // CPU 양보
#define SYS_GETTICK    4   // 시스템 틱 조회
//#define SYS_GETPID     5   // 현재 프로세스 PID 조회
//#define SYS_EXEC       6   // 프로그램 실행 (바이너리 로드)
//#define SYS_KILL       7   // 프로세스 강제 종료
//#define SYS_PS         8   // 실행 중인 태스크 목록 조회

// 10 ~ 19: 파일 디스크립터 & I/O (파이프, 리다이렉션)
#define SYS_READ       10  // 파일/stdin 읽기
#define SYS_WRITE      11  // 파일/stdout/stderr 쓰기
#define SYS_OPEN       12  // 파일 열기
#define SYS_CLOSE      13  // 파일 닫기
//#define SYS_DUP2       14  // FD 복제 (I/O 리다이렉션 > 의 핵심!)
//#define SYS_PIPE       15  // 파이프 생성 (| 의 핵심!)

// 20 ~ 29: 파일 시스템 & 디렉터리 제어
#define SYS_GETCWD     20  // 현재 작업 경로 조회 (pwd, 프롬프트)
#define SYS_CHDIR      21  // 현재 작업 경로 변경 (cd)
//#define SYS_MKDIR      22  // 디렉터리 생성 (mkdir)
//#define SYS_UNLINK     23  // 파일 삭제 (rm)
//#define SYS_READDIR    24  // 디렉터리 엔트리 순회 (ls)

// C++ 시스템 콜 핸들러 (boot.asm의 isr80에서 호출)
extern "C" void syscall_handler(Registers* regs);

// 호출자 편의를 위한 래퍼 함수 (소프트웨어 인터럽트 int 0x80 발생)
void sys_exit();
void sys_sleep(unsigned int delay_ticks);
void sys_yield();
unsigned int sys_get_tick();
int sys_read(int fd, void* buf, unsigned int count);
int sys_write(int fd, const void* buf, unsigned int count);
int sys_open(const char* pathname);
int sys_close(int fd);
int sys_getcwd(char* buf, unsigned int size);
int sys_chdir(const char* path);
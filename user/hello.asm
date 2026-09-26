[bits 32]
[org 0x40000000]       ; 유저 코드 시작 주소

start:
    mov eax, 11        ; SYS_WRITE
    mov ebx, 1         ; fd = 1 (stdout)
    mov ecx, msg       ; buffer
    mov edx, 35        ; length
    int 0x80

    mov eax, 2         ; SYS_SLEEP
    mov ebx, 1000       ; 1000 ms
    int 0x80

    mov eax, 11
    mov ebx, 1
    mov ecx, done_msg
    mov edx, 24
    int 0x80

    ; sys_exit() -> 프로세스 종료
    mov eax, 1         ; SYS_EXIT
    int 0x80

msg:      db "[User Task] Hello from Ring 3 app!", 10  ; 10은 줄바꿈
done_msg: db "[User Task] Bye bye! :)", 10

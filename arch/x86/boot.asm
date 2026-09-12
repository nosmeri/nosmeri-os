; NASM 문법 기준
MBOOT_ALIGN      equ 1 << 0
MBOOT_MEMINFO    equ 1 << 1
MBOOT_FLAGS      equ MBOOT_ALIGN | MBOOT_MEMINFO
MBOOT_MAGIC      equ 0x1BADB002
MBOOT_CHECKSUM   equ -(MBOOT_MAGIC + MBOOT_FLAGS)

section .multiboot
align 4 ; 멀티부트로더가 4바이트씩 뛰면서 읽음 -> 4의 배수에 위치시킴
    dd MBOOT_MAGIC
    dd MBOOT_FLAGS
    dd MBOOT_CHECKSUM

section .bss ; .bss는 메모리에 로드시 0으로 초기화되는 영역 실제 파일에 0이 있지 않음
align 16
stack_bottom:
    resb 16384 ; 16KB 커널 스택 확보
stack_top:

section .text
global _start ; 외부에서 접근할 수 있게
extern kernel_main ; cpp 파일에 선언된 함수 

_start:
    mov esp, stack_top ; 스택 포인터 설정

    ; Multiboot 부트로더(GRUB/QEMU)가 전달해준 정보 스택에 푸시
    push ebx           ; 2번째 인자: multiboot_info 구조체 물리 주소
    push eax           ; 1번째 인자: 멀티부트 매직 넘버 (0x2BADB002)

    call kernel_main   ; cpp 영역으로 진입 (kernel_main(magic, mbi))
    ; 커널에서 비정상적으로 빠져나올시
    cli                ; 모든 인터럽트 무시
.hang:
    hlt ; 외부 인터럽트 발생까지 cpu 일시 정지
    jmp .hang ; 반복

global gdt_flush

gdt_flush:
    mov eax, [esp + 4]  ; cpp에서 인자로 넘겨준 gdt_record의 주소를 가져옴
    lgdt [eax]          ; GDT 로드

    ; 데이터 세그먼트 레지스터들을 커널 데이터 오프셋(0x10)으로 업데이트
    ; 0x10이면 index 2 세그먼트인 kernel data segment
    mov ax, 0x10 
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; 코드 세그먼트(CS)는 직접 수정이 불가능하므로, 기계어 수준에서 'Far Jump'를 수행하여 CS를 0x08(커널 코드 오프셋)로 변경합니다.
    ; 프리패치 큐 제거
    jmp 0x08:.flush

.flush:
    ret ; return

global idt_load
global isr0
global isr14
global irq0
global irq1
global isr80
extern isr_handler
extern page_fault_handler
extern timer_handler
extern keyboard_handler
extern syscall_handler

; IDTR 레지스터 로드
idt_load:
    mov eax, [esp + 4]
    lidt [eax]
    ret

; 0번 예외(Divide by Zero) 스탑
isr0:
    pusha           ; 모든 범용 레지스터 백업 (EAX, ECX, EDX 등)
    call isr_handler ; C++ 실제 핸들러 호출
    popa            ; 레지스터 복구
    iret            ; 인터럽트 복귀 (iret은 인터럽트용 특수 리턴 명령어입니다)

; 14번 예외(Page Fault) 스탑
; 주의: CPU가 자동으로 스택에 에러 코드(Error Code)를 푸시함
isr14:
    pusha           ; 범용 레지스터 백업
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10    ; 커널 데이터 세그먼트 로드
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; 스택: [esp] = gs .. [esp+12] = ds, [esp+16] = pusha(edi..eax), [esp+48] = error_code
    mov eax, [esp + 48] ; CPU가 넣어준 에러 코드 가져오기
    push eax            ; 1번째 인자: error_code
    call page_fault_handler
    add esp, 4          ; 인자 정리

    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 4          ; CPU가 푸시했던 에러 코드(4바이트) 제거 후 iret 해야 함!
    iret

; 0번 인터럽트(타이머) 스탑
irq0:
    pusha           ; 모든 레지스터 백업
    call timer_handler
    popa            ; 레지스터 복구
    iret

; 1번 인터럽트(키보드) 스탑
irq1:
    pusha           ; 모든 레지스터 백업
    call keyboard_handler
    popa            ; 레지스터 복구
    iret

isr80:
    pusha           ; 1. 범용 레지스터 백업 (EAX ~ EDI)
    push ds         ; 2. 유저의 세그먼트 레지스터 백업
    push es
    push fs
    push gs
    mov ax, 0x10    ; 3. GDT 0x10 = 커널 데이터 세그먼트
    mov ds, ax      ; 커널 세그먼트로 전환 (커널 전역변수/메모리 안전 접근 보장)
    mov es, ax
    mov fs, ax
    mov gs, ax
    push esp        ; 스택 포인터를 핸들러의 인자(Registers*)로 전달
    call syscall_handler
    add esp, 4
    pop gs          ; 4. 원래 유저 세그먼트 값으로 복구
    pop fs
    pop es
    pop ds
    popa            ; 5. 원래 범용 레지스터 복구
    iret            ; 6. 유저 모드로 안전하게 복귀


section .note.GNU-stack noalloc noexec nowrite progbits ; ld: warning: boot.o: missing .note.GNU-stack section implies executable stack 경고 없애기
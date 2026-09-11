ENTRY(_start)

SECTIONS
{
    . = 1M; /* 1MB 위치에서 커널 배치 시작 */

    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.multiboot) /* 멀티부트 헤더가 파일 최상단에 와야 합니다 */
        *(.text)
    }

    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata)
    }

    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data)
    }

    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON) /* 컴파일러가 여러 파일에서 공통으로 사용될 수 있는 초기화되지 않은 변수들을 임시로 두는 곳 */
        *(.bss)
    }

    kernel_end = .; /* 커널 바이너리가 끝나는 물리 주소 심볼 */
}
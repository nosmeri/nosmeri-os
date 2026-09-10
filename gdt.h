#pragma once

// 컴파일러의 패딩을 방지하여 정확히 8바이트를 유지
struct gdt_entry {
    unsigned short limit_low;     // 세그먼트 크기 (하위 16비트)
    unsigned short base_low;      // 세그먼트 시작 주소 (하위 16비트)
    unsigned char  base_middle;   // 세그먼트 시작 주소 (중간 8비트)

    unsigned char  access;        // 액세스 권한 및 링 레벨 (8비트)
    /*
    Bit 7,Present,P,이 세그먼트가 물리 메모리에 존재하는지 나타냄. 가상 메모리 스와핑에 쓰이지만, 커널 세그먼트는 항상 메모리에 있으므로 무조건 1로 설정.
    Bit 6~5,Privilege Level,DPL,2비트로 CPU 권한 레벨(Ring)을 정함.커널 세그먼트는 00 (Ring 0), 유저 프로그램은 11 (Ring 3)으로 설정.
    Bit 4,Descriptor Type,S,1이면 일반적인 코드/데이터 세그먼트.0이면 시스템 세그먼트(TSS, LDT 등 특수 목적)를 의미.
    Bit 3,Executable,E,1이면 기계어를 실행할 수 있는 코드 세그먼트.0이면 변수 등을 담는 데이터 세그먼트.
    Bit 2,Direction/Conforming,DC,코드(E=1): 하위 권한(Ring 3)이 이 코드로 점프할 수 있는지, 보통 0. 데이터(E=0): 스택처럼 주소가 아래로 자라는지, 보통 0.
    Bit 1,Readable/Writable,RW,코드(E=1): 코드를 읽을 수 있는지 여부. 실행은 당연히 되지만 데이터를 읽으려면 1로 설정해야 함.- 데이터(E=0): 변수에 값을 쓸 수 있는지 여부. 당연히 써야 하므로 1로 설정.
    Bit 0,Accessed,A,CPU가 이 세그먼트를 사용하면 하드웨어적으로 알아서 1로 바꿈. 우리가 처음 만들 때는 0으로 비워둠.
    */

    unsigned char  granularity;   // 세그먼트 크기(0~3 상위 4비트) 및 플래그(4~7 4비트)
    /*
    플래그
    Bit 3,Granularity,G,Limit의 단위 설정. Limit을 4KB 단위로 계산하여 4GB 메모리를 다 쓰기 위해 1로 설정.
    Bit 2,Size,DB,CPU가 명령어를 16비트로 처리할지 32비트로 처리할지 정함.우리는 32비트 보호 모드를 쓰므로 1 (32비트 모드)로 설정.
    Bit 1,Long Mode,L,64비트 코드를 위한 플래그. 32비트 커널에서는 사용하지 않으므로 0으로 둠.
    Bit 0,Available,AVL,시스템 소프트웨어(커널 개발자)가 마음대로 써도 되는 여분 비트. 보통 안 쓰므로 0으로 비워둠.
    */

    unsigned char  base_high;     // 세그먼트 시작 주소 (상위 8비트)
} __attribute__((packed));

// CPU의 GDTR 레지스터에 넘겨줄 6바이트 포인터 구조체
struct gdt_ptr {
    unsigned short limit;         // GDT 테이블의 총 크기 - 1 (2바이트)
    unsigned int   base;          // GDT 테이블이 시작되는 물리 주소 (4바이트)
} __attribute__((packed));

void init_gdt();
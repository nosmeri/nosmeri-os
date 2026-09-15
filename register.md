# x86 아키텍처(32비트) 핵심 레지스터 정리

운영체제(OS) 커널 개발 및 시스템 프로그래밍에서 필수적으로 다루는 32비트(IA-32) 레지스터들을 역할별로 분류한 문서입니다.

---

## 1. 범용 레지스터 (General-Purpose Registers)

산술/논리 연산, 데이터 이동, 메모리 주소 지정 등에 자유롭게 사용되는 레지스터입니다.

| 레지스터 | 명칭 | 커널 개발 및 주요 역할 |
| :--- | :--- | :--- |
| **`EAX`** | Accumulator | 산술/논리 연산, **함수 반환값 저장**, 시스템 콜 번호 지정 |
| **`EBX`** | Base | 메모리 주소 지정 베이스, **부팅 시 `multiboot_info` 포인터 전달** |
| **`ECX`** | Counter | 반복문 카운터(`loop`), 메모리 복사/초기화 반복 횟수 지정 |
| **`EDX`** | Data | 곱셈/나눗셈 보조, **하드웨어 I/O 포트 번호 지정** (`in`/`out`) |
| **`ESI`** | Source Index | 문자열/메모리 복사 시 출발지(Source) 주소 포인터 |
| **`EDI`** | Destination Index | 문자열/메모리 작업 시 목적지(Destination) 주소 포인터 |
| **`ESP`** | Stack Pointer | **현재 스택의 맨 꼭대기(Top) 주소** (`push`, `pop`, `call` 시 자동 갱신) |
| **`EBP`** | Base Pointer | **현재 함수 스택 프레임의 기준점** (지역 변수 및 매개변수 참조용) |

---

## 2. 명령어 포인터 (Instruction Pointer)

* **`EIP` (Extended Instruction Pointer)**
  * CPU가 **다음에 실행할 기계어 명령어의 주소**를 가리킵니다.
  * 프로그래머가 직접 값을 대입할 수 없으며, `jmp`, `call`, `ret` 등의 분기 명령어에 의해 간접적으로 변경됩니다.

---

## 3. 플래그 레지스터 (EFLAGS)

연산 결과와 CPU 동작 제어 상태를 비트 단위로 저장합니다.

### 상태 플래그 (Status Flags)

* **`ZF` (Zero Flag, Bit 6):** 연산 결과가 0이거나 비교 값이 같을 때 1로 세트됩니다.
* **`CF` (Carry Flag, Bit 0):** 부호 없는 덧셈/뺄셈에서 올림/빌림 발생 시 세트됩니다.
* **`SF` (Sign Flag, Bit 7):** 연산 결과가 음수(최상위 비트가 1)일 때 세트됩니다.

### 시스템 제어 플래그 (System Control Flags)

* **`IF` (Interrupt Enable Flag, Bit 9):** 외부 하드웨어 인터럽트 허용 여부를 결정합니다. (`sti` 명령어로 1, `cli` 명령어로 0 세트)
* **`DF` (Direction Flag, Bit 10):** 연속 메모리 작업 시 주소의 증가(`cld`) 또는 감소(`std`) 방향을 결정합니다.

---

## 4. 세그먼트 레지스터 (Segment Registers)

GDT(글로벌 디스크립터 테이블)에 정의된 특정 세그먼트를 가리키는 **16비트 셀렉터(Selector)**를 담습니다. 보호 모드에서는 메모리 권한(Ring 0 ~ 3)을 결정하는 중요한 역할을 합니다.

| 레지스터 | 이름 | 역할 |
| :--- | :--- | :--- |
| **`CS`** | Code Segment | 현재 실행 중인 코드가 위치한 세그먼트 (`EIP`와 세트로 동작) |
| **`DS`** | Data Segment | 전역 변수 등 일반 데이터가 위치한 세그먼트 |
| **`SS`** | Stack Segment | 스택(`ESP`, `EBP`)이 위치한 세그먼트 |
| **`ES`, `FS`, `GS`** | Extra Segments | 추가 데이터 세그먼트 (스레드 데이터 참조 등에 활용) |

---

## 5. 제어 레지스터 (Control Registers)

하드웨어의 핵심 동작 모드(보호 모드, 페이징 등)를 켜고 끄는 특수 레지스터로, 커널 모드(Ring 0)에서만 접근 가능합니다.

* **`CR0`:** 시스템 제어 스위치
  * **Bit 0 (PE):** `1`로 세트 시 **32비트 보호 모드(Protected Mode)** 진입
  * **Bit 31 (PG):** `1`로 세트 시 **가상 메모리(페이징)** 활성화
* **`CR2`:** 페이징 활성화 상태에서 Page Fault 발생 시, **오류를 일으킨 가상 주소**가 저장됩니다.
* **`CR3` (PDBR):** 최상위 **페이지 디렉토리(Page Directory)의 물리 메모리 시작 주소**를 담습니다. 프로세스 컨텍스트 스위칭 시 이 값이 변경됩니다.
* **`CR4`:** 대용량 페이지(PSE) 등 확장 하드웨어 기능을 제어합니다.

---

## 6. 시스템 테이블 레지스터 (Descriptor Table Registers)

커널이 메모리에 구축해 둔 주요 하드웨어 테이블의 물리적 위치와 크기를 CPU에게 알려줍니다.

* **`GDTR`:** **GDT(Global Descriptor Table)**의 시작 주소와 크기 보관 (`lgdt` 명령어로 로드)
* **`IDTR`:** **IDT(Interrupt Descriptor Table)**의 시작 주소와 크기 보관 (`lidt` 명령어로 로드)
* **`TR` (Task Register):** 유저 모드에서 커널 모드로 진입 시 참조할 **TSS(Task State Segment)** 셀렉터 보관 (`ltr` 명령어로 로드)

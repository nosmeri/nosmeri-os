# NOSMERI-OPERATING-SYSTEM

## Overview

x86 32비트 아키텍처 기반으로 직접 설계하고 구현한 학습용 운영체제

---

## 빌드 및 실행 방법 (How to Run)

```bash
# 디스크 포멧
qemu-img create -f raw disk.img 16M

# 빌드 및 QEMU 에뮬레이터 실행
make run

# 변경된 파일만 빌드
make

# 빌드 산출물(build/) 정리
make clean
```

---

## TODO

- [x] GDT & IDT 및 하드웨어 인터럽트 (PIC, PIT, 키보드)
- [x] 물리 메모리 관리자 (PMM)
- [x] 가상 메모리 관리자 (VMM 페이징)
- [x] 커널 힙 메모리 관리자 (kmalloc / kfree)
- [x] 시스템 콜 (int 0x80)
- [x] 멀티태스킹 (태스크 제어 블록 & 라운드 로빈 스케줄러)
- [x] 셸 연동 및 프로세스 모니터링 (ps 명령어)
- [x] Non-blocking Task Sleep
- [x] task kill
- [x] 키보드 버퍼 큐 + 셸 태스크 분리
- [x] exit task
- [x] 파일시스템 (가상 파일 시스템 VFS)
- [x] simplefs 계층에서 print_string 하는 부분 수정
- [x] User Mode(Ring 3) 전환
- [x] 프로세스별 독립 페이지 디렉터리(CR3 분리) -> 현재 모든 페이지가 유저모드인 문제 해결
- [x] 디스크에 저장된 실행파일 로더
- [ ] 파일 삭제
- [ ] File Descriptor
- [ ] 쉘을 유저프로세스로 전환

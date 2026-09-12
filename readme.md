# NOSMERI-OPERATING-SYSTEM

## Overview

**NOSMERI-OS**는 x86 32비트 아키텍처 기반으로 밑바닥부터 직접 설계하고 구현한 학습용 운영체제입니다.

---

## 빌드 및 실행 방법 (How to Run)

```bash
# 빌드 및 QEMU 에뮬레이터 실행
make run
# 또는
./run

# 변경된 파일만 빌드
make

# 빌드 산출물(build/) 정리
make clean
```

---

## TODO

- [x] Makefile 도입 및 빌드 시스템 최적화
- [ ] 멀티태스킹 (태스크 제어 블록 & 라운드 로빈 스케줄러)
- [ ] 파일시스템 (가상 파일 시스템 VFS & Ramdisk)

# 컴파일러 및 도구 설정
CXX = g++
AS = nasm
LD = ld
QEMU = qemu-system-i386

# 플래그 설정
CXXFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra -fno-exceptions -fno-rtti -fno-use-cxa-atexit \
           -Iarch/x86 -Idrivers -Imm -Ilib -Ishell -Itask
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T arch/x86/linker.ld

# 빌드 디렉토리
BUILD_DIR = build

# 소스 파일 목록
ASM_SRCS = arch/x86/boot.asm
CPP_SRCS = kernel.cpp \
           arch/x86/gdt.cpp \
           arch/x86/idt.cpp \
           arch/x86/syscall.cpp \
           drivers/pic.cpp \
           drivers/timer.cpp \
           drivers/keyboard.cpp \
           drivers/vga.cpp \
           mm/pmm.cpp \
           mm/vmm.cpp \
           mm/heap.cpp \
           task/task.cpp \
           lib/string.cpp \
           shell/shell.cpp

# 오브젝트 파일 목록 (build/ 디렉토리 내에 동일한 폴더 구조로 생성)
ASM_OBJS = $(patsubst %.asm, $(BUILD_DIR)/%.o, $(ASM_SRCS))
CPP_OBJS = $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(CPP_SRCS))
OBJS = $(ASM_OBJS) $(CPP_OBJS)

# 최종 커널 바이너리
TARGET = $(BUILD_DIR)/mykernel.bin

# 기본 타겟
all: $(TARGET)

# 링킹 규칙
$(TARGET): $(OBJS)
	@echo "[LD] $@"
	@$(LD) $(LDFLAGS) -o $@ $(OBJS)

# C++ 컴파일 규칙 (변경된 파일만 컴파일)
$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "[CXX] $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# 어셈블리 컴파일 규칙
$(BUILD_DIR)/%.o: %.asm
	@mkdir -p $(dir $@)
	@echo "[AS]  $<"
	@$(AS) $(ASFLAGS) $< -o $@

# QEMU 실행 타겟 (빌드 후 바로 실행)
run: $(TARGET)
	@echo "[QEMU] Launching OS..."
	$(QEMU) -m 3G -kernel $(TARGET)

# 빌드 산출물 정리
clean:
	@echo "[CLEAN] Removing $(BUILD_DIR)..."
	@rm -rf $(BUILD_DIR)

.PHONY: all run clean

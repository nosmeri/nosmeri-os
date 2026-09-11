#pragma once

// I/O 포트 1바이트 쓰기
static inline void outb(unsigned short port, unsigned char val) {
    __asm__ __volatile__ ("outb %0, %1" : : "a"(val), "Nd"(port));
}

// I/O 포트 1바이트 읽기
static inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    __asm__ __volatile__ ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// I/O 포트 2바이트(워드) 쓰기
static inline void outw(unsigned short port, unsigned short val) {
    __asm__ __volatile__ ("outw %0, %1" : : "a"(val), "Nd"(port));
}

// I/O 포트 2바이트(워드) 읽기
static inline unsigned short inw(unsigned short port) {
    unsigned short ret;
    __asm__ __volatile__ ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// I/O 대기 (느린 하드웨어 포트 I/O 동기화용)
static inline void io_wait() {
    __asm__ __volatile__ ("outb %%al, $0x80" : : "a"(0));
}

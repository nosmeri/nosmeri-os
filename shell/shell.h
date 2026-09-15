#pragma once

struct Command {
    const char* name;
    const char* description;
    void (*handler)(const char* arg);  // 함수 포인터 : void를 반환하고 char* arg를 인자로 받는 handler라는 이름의 포인터
};

// 쉘 인터페이스
void shell_main();
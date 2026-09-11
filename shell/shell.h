#pragma once

#include "string.h"

// 문자열 비교 (헤더에서 공용 사용)
int strcmp(const char* s1, const char* s2);

// 쉘 인터페이스
void shell_init();
void print_prompt();
void execute_command(const char* cmd);
void shell_handle_key(char c);

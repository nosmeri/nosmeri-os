#pragma once

#include "string.h"

// 쉘 인터페이스
void shell_init();
void print_prompt();
void execute_command(const char* cmd);
void shell_handle_key(char c);
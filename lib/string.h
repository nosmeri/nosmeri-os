#pragma once

void reverse_string(char *str, int length);
void itoa(unsigned int num, char *str, int base);
void itoa(int num, char *str, int base);
int atoi(const char* str);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, unsigned int n);
void* memset(void* dest, int val, unsigned int count);
void* memcpy(void* dest, const void* src, unsigned int count);
unsigned int strlen(const char* str);
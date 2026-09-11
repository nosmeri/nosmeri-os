#include "string.h"

// 문자열을 뒤집어주는 헬퍼 함수
void reverse_string(char *str, int length) {
    int start = 0;
    int end = length - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

// 숫자를 문자열로 변환하는 함수
// num: 변환할 숫자, str: 결과가 저장될 버퍼, base: 진법 (10진수=10, 16진수=16)
void itoa(unsigned int num, char *str, int base) {
    int i = 0;

    // 0인 경우의 예외 처리
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    // 일의 자리부터 하나씩 추출하여 문자로 변환
    while (num != 0) {
        int remainder = num % base;
        
        // 16진수를 위해 나머지가 9보다 크면 'A'~'F'로 변환, 아니면 '0'~'9'로 변환
        if (remainder > 9) {
            str[i++] = (remainder - 10) + 'A';
        } else {
            str[i++] = remainder + '0';
        }
        num = num / base;
    }

    str[i] = '\0'; // 문자열의 끝을 알리는 널(Null) 문자 삽입

    // 현재 버퍼에는 역순으로(예: 123 -> "321") 들어있으므로 뒤집어줌
    reverse_string(str, i);
}




// 숫자를 문자열로 변환하는 함수
// num: 변환할 숫자, str: 결과가 저장될 버퍼, base: 진법 (10진수=10, 16진수=16)
void itoa(int num, char *str, int base) {
    int i = 0;
    char is_negative = false;

    // 0인 경우의 예외 처리
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    // 10진수이면서 음수인 경우 처리
    if (num < 0 && base == 10) {
        is_negative = true;
        num = -num;
    }

    // 일의 자리부터 하나씩 추출하여 문자로 변환
    while (num != 0) {
        int remainder = num % base;
        
        // 16진수를 위해 나머지가 9보다 크면 'A'~'F'로 변환, 아니면 '0'~'9'로 변환
        if (remainder > 9) {
            str[i++] = (remainder - 10) + 'A';
        } else {
            str[i++] = remainder + '0';
        }
        num = num / base;
    }

    // 음수였으면 마지막에 '-' 기호 추가
    if (is_negative) {
        str[i++] = '-';
    }

    str[i] = '\0'; // 문자열의 끝을 알리는 널(Null) 문자 삽입

    // 현재 버퍼에는 역순으로(예: 123 -> "321") 들어있으므로 뒤집어줌
    reverse_string(str, i);
}
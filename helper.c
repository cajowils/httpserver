#include <stdlib.h>
#include <stdint.h>
#include <stdint.h>
#include <ctype.h>

uint16_t strtouint16(char number[]) {
    char *last;
    long num = strtol(number, &last, 10);
    if (num <= 0 || num > UINT16_MAX || *last != '\0') {
        return 0;
    }
    return num;
}

//takes the professor's function and does the same thing but for ints to allow for larger numbers

int strtoint(char number[]) {
    char *last;
    long num = strtol(number, &last, 10);
    if (num <= 0 || num > INT64_MAX || *last != '\0') {
        return 0;
    }
    return num;
}

int is_number(char *str, int size) {
    for (int num = 0; num < size; num++) { // Makes sure the value of Content-Length is a number
        if (!isdigit(str[num])) {
            return 0;
        }
    }
    return 1;
}

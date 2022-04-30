#include <stdlib.h>
#include <stdint.h>
#include <stdint.h>

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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <err.h>

int main(int argc, char **argv) {
    
    char *delimiter = argv[1];

    //check if delimiter is > 1 char, otherwise use warn or err

    if (strlen(delimiter) > 1) {
        warn("Cannot handle multi-character splits: %s\nusage: ./split: <split_char> [<file1> <file2> ...]", delimiter);
        return 0;
    }

    printf("Delimiter: %lu\n", strlen(delimiter));

    for (int i=2; i < argc; i++) {
        char buffer[100];
        int size = snprintf(buffer, 100, "File: %s\n", argv[i]);
        write(1, buffer, size);

    //syscall(SYS_write, 1, , 120);
    }

    return 0;
}

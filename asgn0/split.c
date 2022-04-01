#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <err.h>

int main(int argc, char **argv) {
    
    if (argc < 3) {
        printf("Not enough arguments\nusage: ./split: <split_char> [<file1> <file2> ...]\n");
        return 0;
    }

    char *delimiter = argv[1];

    //check if delimiter is > 1 char, otherwise use warn or err

    if (strlen(delimiter) > 1) {
        printf("Cannot handle multi-character splits: %s\nusage: ./split: <split_char> [<file1> <file2> ...]\n", delimiter);
        return 0;
    }
    //iterate through files, read them in, and write the version that is split by the delimiter
    for (int i=2; i < argc; i++) {

        if (strcmp(argv[i], "-") == 0) {
            printf("read from stdin\n");
        }

        else if (access(argv[i], F_OK) == 0) {
            printf("file exists\n");
            char buffer[100];
            int size = snprintf(buffer, 100, "File: %s\n", argv[i]);
            write(1, buffer, size);
        }
        else {
            warn("%s", argv[i]);
        }

        

    //syscall(SYS_write, 1, , 120);
    }

    return 0;
}

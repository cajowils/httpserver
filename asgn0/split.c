#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <err.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> 

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

        int fd = open(argv[i], O_RDONLY);

        if (strcmp(argv[i], "-") == 0) {
            printf("read from stdin\n");
        }
        else if (fd != -1) {
            char *buf = (char *) calloc(100, sizeof(char));
            int size = read(fd, buf, 1);
            while (size > 0) {
                //int size = snprintf(buf, 100, "File: %s\n", argv[i]);
                if (strcmp(delimiter, buf) == 0) {
                    write(1, "\n", strlen("\n"));
                }
                else {
                    write(1, buf, size);
                }
                size = read(fd, buf, 1);
            }
            
        }
        else {
            warn("%s", argv[i]);
        }
    }

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <err.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

int replace(int fd, char *delimiter) {
    int bytes = 10000000;
    char *buf = (char *) calloc(bytes, sizeof(char));
    int size;

    while ((size = read(fd, buf, bytes)) > 0) {
        for (int i = 0; i < size; i++) {
            buf[i] = (delimiter[0] == buf[i]) ? '\n' : buf[i];
        }
        write(1, buf, size);
        if (errno == 28) {
            warnx("No space left on device");
            return 28;
        }
    }
    free(buf);
    return 0;
}

int main(int argc, char **argv) {

    if (argc < 3) {
        errx(22, "Not enough arguments\nusage: ./split: <split_char> [<file1> <file2> ...]");
        return 0;
    }

    char *delimiter = argv[1];

    //check if delimiter is > 1 char, otherwise use warn or err

    if (strlen(delimiter) > 1) {
        errx(22, "Cannot handle multi-character splits: %s\nusage: ./split: <split_char> [<file1> <file2> ...]",delimiter);
        return 0;
    }
    //iterate through files, read them in, and write the version that is split by the delimiter
    for (int i = 2; i < argc; i++) {

        int fd = open(argv[i], O_RDONLY);

        if (strcmp(argv[i], "-") == 0) {
            replace(0, delimiter);
        } else if (fd != -1) {
            replace(fd, delimiter);

        } else {
            warn("%s", argv[i]);
        }
    }

    if (errno == 28) {
        return errno;
    }
    return 0;
}

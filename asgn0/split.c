#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <fcntl.h>
#include <errno.h>

int replace(int fd, char *delimiter) {

    // Initializes buffer with size of 1 MB
    int bytes = 1000000;
    char *buf = (char *) calloc(bytes, sizeof(char));
    int size;

    // Reads in bytes from file
    while ((size = read(fd, buf, bytes)) > 0) {
        // Iterates over each byte read in, replacing each instance of delimiter with a newline
        for (int i = 0; i < size; i++) {
            buf[i] = (delimiter[0] == buf[i]) ? '\n' : buf[i];
        }
        if (write(1, buf, size) < 0) {
            errx(28, "No space left on device");
        }
    }
    free(buf);
    return 0;
}

int main(int argc, char **argv) {

    // Check if there are the appropriate number of arguments

    if (argc < 3) {
        errx(22, "Not enough arguments\nusage: ./split: <split_char> [<file1> <file2> ...]");
    }

    char *delimiter = argv[1];

    // Check if delimiter is > 1 char, otherwise use warn or err

    if (strlen(delimiter) > 1) {
        errx(22,
            "Cannot handle multi-character splits: %s\nusage: ./split: <split_char> [<file1> "
            "<file2> ...]",
            delimiter);
    }

    // Remembers if a file could not be accessed
    int fail = 0;

    for (int i = 2; i < argc; i++) {
        int fd;
        if (strcmp(argv[i], "-") == 0) {                    // Loads input from stdin
            replace(0, delimiter);
            close(0);
        } else if ((fd = open(argv[i], O_RDONLY)) > 0) {    // Loads input from successful file
            replace(fd, delimiter);
            close(fd);
        } else {                                            // File not available, sending warning
            fail = 1;
            warn("%s", argv[i]);
        }
    }
    if (fail == 1) {
        return errno;
    }
    return 0;
}

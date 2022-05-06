#include <err.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <regex.h>

#include "requests.h"
#include "response.h"
#include "helper.h"
#include "list.h"


#define OPTIONS              "t:l:"
#define BUF_SIZE             4096
#define DEFAULT_THREAD_COUNT 4

static FILE *logfile;
#define LOG(...) fprintf(logfile, __VA_ARGS__);

void send_response(struct response rsp, int connfd) {

    int line_size = (int) strlen(rsp.line.version) + (int) strlen(rsp.line.phrase)
                    + 8; //need 3 for the status code, 2 for spaces and 2 for the carriage return
    char *line_buf = (char *) calloc(1, sizeof(char) * line_size);
    snprintf(line_buf, line_size, "%s %d %s\r\n", rsp.line.version, rsp.line.code, rsp.line.phrase);
    write(connfd, line_buf, line_size - 1);
    free(line_buf);

    Node *ptr = rsp.headers->next;
    while (ptr != NULL) {
        int header_size = (int) strlen(ptr->head) + (int) strlen(ptr->val)
                          + 5; //1 for colon, 1 for space, 2 for \r\n
        char *header_buf = (char *) calloc(1, sizeof(char) * header_size);
        snprintf(header_buf, header_size, "%s: %s\r\n", ptr->head, ptr->val);
        write(connfd, header_buf, header_size - 1);
        ptr = ptr->next;
        free(header_buf);
    }
    char *creturn = "\r\n";
    write(connfd, creturn, strlen(creturn));

    if (rsp.mode == 0 && rsp.line.code == 200) { //checks that it is a successful GET request
        int size;
        int bytes = 2048;
        char buf2[bytes];
        while ((size = read(rsp.fd, buf2, bytes)) > 0) {
            write(connfd, buf2, size);
        }
    } else {

        int phrase_size = (int) strlen(rsp.line.phrase) + 2; //1 for /n
        char *phrase_buf = (char *) calloc(1, sizeof(char) * phrase_size);
        snprintf(phrase_buf, phrase_size, "%s\n", rsp.line.phrase);
        write(connfd, phrase_buf, phrase_size - 1);
        free(phrase_buf);
    }

    return;
}

// Creates a socket for listening for connections.
// Closes the program and prints an error message on error.
static int create_listen_socket(uint16_t port) {
    struct sockaddr_in addr;
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        err(EXIT_FAILURE, "socket error");
    }
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htons(INADDR_ANY);
    addr.sin_port = htons(port);
    if (bind(listenfd, (struct sockaddr *) &addr, sizeof addr) < 0) {
        err(EXIT_FAILURE, "bind error");
    }
    if (listen(listenfd, 128) < 0) {
        err(EXIT_FAILURE, "listen error");
    }
    return listenfd;
}

/*static void handle_connection(int connfd) {
    char buf[BUF_SIZE];
    ssize_t bytes_read, bytes_written, bytes;
    do {
        // Read from connfd until EOF or error.
        bytes_read = read(connfd, buf, sizeof(buf));
        if (bytes_read < 0) {
            return;
        }

        // Write to stdout.
        bytes = 0;
        do {
            bytes_written = write(STDOUT_FILENO, buf + bytes, bytes_read - bytes);
            if (bytes_written < 0) {
                return;
            }
            bytes += bytes_written;
        } while (bytes_written > 0 && bytes < bytes_read);

        // Write to connfd.
        bytes = 0;
        do {
            bytes_written = write(connfd, buf + bytes, bytes_read - bytes);
            if (bytes_written < 0) {
                return;
            }
            bytes += bytes_written;
        } while (bytes_written > 0 && bytes < bytes_read);
    } while (bytes_read > 0);
}*/

void finish_writing(struct request req, struct response rsp, int fd) {
    int bytes = 4096;
    int size = 0;
    char *buf = (char *) calloc(1, sizeof(char) * bytes);
    int bytes_written = 0;

    int read_bytes
        = (req.body_size - req.body_read > bytes) ? bytes : req.body_size - req.body_read;
    do {
        size = read(fd, buf, read_bytes);
        bytes_written = write(rsp.fd, buf, size);
        req.body_read += size;
        read_bytes
            = (req.body_size - req.body_read > bytes) ? bytes : req.body_size - req.body_read;
    } while (size > 0 && req.body_read < req.body_size);
    free(buf);

    return;
}

int read_all(int fd, char *buf, int nbytes) {
    int total = 0;
    int bytes = 0;
    const char *pattern = "\r\n\r\n";
    regex_t re;
    if (regcomp(&re, pattern, REG_EXTENDED) != 0) {
        bytes = 0;
        //return 500
    }
    do {
        bytes = read(fd, buf + total, 1);
        total += bytes;
    } while (bytes > 0 && total < nbytes && (regexec(&re, buf, 0, NULL, 0) != 0));
    regfree(&re);
    return total;
}

void handle_connection(int connfd) {
    int bytes = 2048;
    char *r = (char *) calloc(1, sizeof(char) * bytes);
    int size = read_all(connfd, r, bytes);

    // parse the buffer for all of the request information and put it in a request struct
    struct request req = parse_request_regex(r, size);

    //process the request and format it into a response
    struct response rsp = process_request(req);
    if ((rsp.line.code == 200 || rsp.line.code == 201)
        && (req.mode == 1
            || req.mode == 2)) { //if it is a successful PUT or APPEND, finish writing to the file
        finish_writing(req, rsp, connfd);
    }

    send_response(rsp, connfd);
    if (req.line.method != NULL && req.line.URI != NULL) {
        LOG("%s,/%s,%d,%d\n", req.line.method, req.line.URI, rsp.line.code, req.ID);
        fflush(logfile);
    }

    free(r);
    delete_request(req);
    delete_response(rsp);
    return;
}

static void sigterm_handler(int sig) {
    if (sig == SIGTERM) {
        warnx("received SIGTERM");
        fclose(logfile);
        exit(EXIT_SUCCESS);
    }
}

static void usage(char *exec) {
    fprintf(stderr, "usage: %s [-t threads] [-l logfile] <port>\n", exec);
}

int main(int argc, char *argv[]) {
    int opt = 0;
    int threads = DEFAULT_THREAD_COUNT;
    logfile = stderr;

    while ((opt = getopt(argc, argv, OPTIONS)) != -1) {
        switch (opt) {
        case 't':
            threads = strtol(optarg, NULL, 10);
            if (threads <= 0) {
                errx(EXIT_FAILURE, "bad number of threads");
            }
            break;
        case 'l':
            logfile = fopen(optarg, "w");
            if (!logfile) {
                errx(EXIT_FAILURE, "bad logfile");
            }
            break;
        default: usage(argv[0]); return EXIT_FAILURE;
        }
    }

    if (optind >= argc) {
        warnx("wrong number of arguments");
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    uint16_t port = strtouint16(argv[optind]);
    if (port == 0) {
        errx(EXIT_FAILURE, "bad port number: %s", argv[1]);
    }

    signal(SIGPIPE, SIG_IGN);
    signal(SIGTERM, sigterm_handler);

    int listenfd = create_listen_socket(port);
    //LOG("port=%" PRIu16 ", threads=%d\n", port, threads);

    for (;;) {
        int connfd = accept(listenfd, NULL, NULL);
        if (connfd < 0) {
            warn("accept error");
            continue;
        }
        handle_connection(connfd);
        close(connfd);
    }

    return EXIT_SUCCESS;
}

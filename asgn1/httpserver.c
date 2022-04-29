#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <regex.h>
#include <ctype.h>
#include <errno.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "requests.h"
#include "response.h"
#include "helper.h"
#include "list.h"

/**
   Converts a string to an 16 bits unsigned integer.
   Returns 0 if the string is malformed or out of the range.
 */

void send_response(struct response rsp, int connfd) {

    int line_size = (int) strlen(rsp.line.version) + (int) strlen(rsp.line.phrase)
                    + 8; //need 3 for the status code, 2 for spaces and 2 for the carriage return
    char *line_buf = (char *) calloc(1, sizeof(char) * line_size);
    snprintf(line_buf, line_size, "%s %d %s\r\n", rsp.line.version, rsp.line.code, rsp.line.phrase);
    write(connfd, line_buf, line_size-1);
    free(line_buf);

    Node *ptr = rsp.headers->next;
    while (ptr != NULL) {
        //printf("%s: %s\r\n", rsp.headers[i].head, rsp.headers[i].val);
        int header_size = (int) strlen(ptr->head) + (int) strlen(ptr->val) + 5;
        char *header_buf = (char *) calloc(1, sizeof(char) * header_size);
        snprintf(header_buf, header_size, "%s: %s\r\n", ptr->head, ptr->val);
        write(connfd, header_buf, header_size-1);
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

        int phrase_size = (int) strlen(rsp.line.phrase) + 2;
        char *phrase_buf = (char *) calloc(1, sizeof(char) * phrase_size);
        snprintf(phrase_buf, phrase_size, "%s\n", rsp.line.phrase);
        write(connfd, phrase_buf, phrase_size-1);
        free(phrase_buf);
    }

    return;
}

/**
   Creates a socket for listening for connections.
   Closes the program and prints an error message on error.
 */
int create_listen_socket(uint16_t port) {
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

    if (listen(listenfd, 500) < 0) {
        err(EXIT_FAILURE, "listen error");
    }

    return listenfd;
}

void finish_writing(struct request req, struct response rsp, int fd) {
    int bytes = 4096;
    int size = 0;
    char buf[bytes];
    int bytes_written = 0;

    int read_bytes
        = (req.body_size - req.body_read > bytes) ? bytes : req.body_size - req.body_read;
    //printf("testing here\n");

    /*if (req.body_read < req.body_size && st.st_size > 0) {

        

        while ((size = read(fd, buf, read_bytes)) > 0) {
            //printf("size: %d\n", size);
            //printf("read_bytes: %d\nbody read: %d\nbody size: %d\n", read_bytes, req.body_read, req.body_size);

            bytes_written = write(rsp.fd, buf, size);
            req.body_read += size;
            read_bytes
                = (req.body_size - req.body_read > bytes) ? bytes : req.body_size - req.body_read;
            if (req.body_read >= req.body_size || bytes_written < bytes) {
                break;
            }
        }
    }*/

    do {
        size = read(fd, buf, read_bytes);
        bytes_written = write(rsp.fd, buf, size);
        req.body_read += size;
        read_bytes
            = (req.body_size - req.body_read > bytes) ? bytes : req.body_size - req.body_read;
    } while (size > 0); //&& bytes_written >= bytes);
    //printf("after write\n");

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


    //printf("before read\n");
    do {
        bytes = read(fd, buf+total, nbytes-total);
        total+=bytes;
        
        //printf("read\n");
    } while (bytes > 0 && total < nbytes && (regexec(&re, buf, 0, NULL, 0) != 0));
    //printf("done reading\n");
    regfree(&re);
    return total;
}

void handle_connection(int connfd) {
    int bytes = 2048;
    char *r = (char *) calloc(1, sizeof(char) * bytes);
    //int size = read(connfd, r, bytes);
    //printf("reading all\n");
    int size = read_all(connfd, r, bytes);
    //printf("request:\n%s\n", r);

    // parse the buffer for all of the request information and put it in a request struct
    //printf("parsing request\n");
    struct request req = parse_request_regex(r, size);
    //printf("processing response\n");

    struct response rsp = process_request(req);
    //printf("finish writing\n");
    if ((rsp.line.code == 200 || rsp.line.code == 201) && (req.mode == 1 || req.mode == 2)) {
        finish_writing(req, rsp, connfd);
    }
    //printf("sending response\n");

    send_response(rsp, connfd);

    // check for errors in the request (wrong version, format, etc) and issue appropriate status
    // send the request to the appropriate method (GET, PUT, APPEND) to deal with the response there
    //printf("code: %d\n", rsp.line.code);
    //printf("Before frees\n");
    free(r);
    //printf("buffer freed\n");
    delete_request(req);
    //printf("request deleted\n");
    delete_response(rsp);
    //printf("response deleted\n");
    return;
}

int main(int argc, char *argv[]) {

    int listenfd;
    uint16_t port;

    if (argc != 2) {
        errx(EXIT_FAILURE, "wrong arguments: %s port_num", argv[0]);
    }

    port = strtouint16(argv[1]);
    if (port == 0) {
        errx(EXIT_FAILURE, "invalid port number: %s", argv[1]);
    }
    listenfd = create_listen_socket(port);

    signal(SIGPIPE, SIG_IGN);

    while (1) {
        errno = 0;
        int connfd = accept(listenfd, NULL, NULL);
        if (connfd < 0) {
            warn("accept error");
            continue;
        }
        handle_connection(connfd);

        // good code opens and closes objects in the same context. *sigh*
        close(connfd);
    }

    return EXIT_SUCCESS;
}

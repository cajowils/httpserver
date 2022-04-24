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

/**
   Converts a string to an 16 bits unsigned integer.
   Returns 0 if the string is malformed or out of the range.
 */

void send_response(struct response rsp, int connfd) {
    
    int size;
    size = (int)strlen(rsp.line.version) + (int)strlen(rsp.line.phrase) + 8; //need 3 for the status code, 2 for epaces and 2 for the carriage return
    char buf[2048];
    snprintf(buf, size, "%s %d %s\r\n", rsp.line.version, rsp.line.code, rsp.line.phrase);
    write(connfd, buf, size);
    for (int i=0; i < rsp.num_headers; i++) {
        //printf("%s: %s\r\n", rsp.headers[i].head, rsp.headers[i].val);
        size = (int)strlen(rsp.headers[i].head) + (int)strlen(rsp.headers[i].val) + 5;
        char buf[size];
        snprintf(buf, size, "%s: %s\r\n", rsp.headers[i].head, rsp.headers[i].val);
        write(connfd, buf, size);
    }
    char *creturn = "\r\n";
    write(connfd, creturn, strlen(creturn));

    if (rsp.mode == 0) {
        int size;
        int bytes = 2048;
        char buf[bytes];
        while ((size = read(rsp.fd, buf, bytes)) > 0) {
            //printf("buf: %s\n", buf);
            write(connfd, buf, size);
        }
        return;
    }

    size = (int)strlen(rsp.line.phrase) + 2;
    char buf2[size];
    snprintf(buf2, size, "%s\n", rsp.line.phrase);
    write(connfd, buf2, size);

        
    
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

void handle_connection(int connfd) {
    int bytes = 2048;
    char *r = (char *) calloc(1, sizeof(char) * bytes);
    read(connfd, r, bytes);
    
    // parse the buffer for all of the request information and put it in a request struct

    

    struct request req = parse_request_regex(r);
    
    struct response rsp = process_request(req);

    send_response(rsp, connfd);

    // check for errors in the request (wrong version, format, etc) and issue appropriate status
    // send the request to the appropriate method (GET, PUT, APPEND) to deal with the response there

    delete_request(req);
    close(rsp.fd);
    free(r);
    //delete_response(rsp);
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

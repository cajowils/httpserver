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

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include "requests.h"
#include "response.h"

struct response status(struct response rsp, int error_code) {
    rsp.line.code = error_code;
    switch (error_code) {
        case 505:
            rsp.line.phrase = "HTTP Version Not Supported\n";
            break;
        case 404:
            rsp.line.phrase = "File Not Found\n";
            break;
    }
    rsp.num_headers = 1;
    strcpy(rsp.headers[0].head, "Content-Length");
    sprintf(rsp.headers[0].val, "%lu", strlen(rsp.headers[0].head));
    return rsp;

}

struct response process_request(struct request req) {
    struct response rsp;
    rsp.line.version = "HTTP/1.1";
    if (strcmp(req.line.version, rsp.line.version) != 0) {
        return status(rsp, 505);
    }

    //look at method and decide if it is acceptable

    //look at the URI and verify that it exists

    //look at version and verify it is the right one

    //read in the headers

    //read the body if indicated by the method

    



    
    return rsp;
    
}


/**
   Converts a string to an 16 bits unsigned integer.
   Returns 0 if the string is malformed or out of the range.
 */
uint16_t strtouint16(char number[]) {
    char *last;
    long num = strtol(number, &last, 10);
    if (num <= 0 || num > UINT16_MAX || *last != '\0') {
        return 0;
    }
    return num;
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
    char *buf = (char *) calloc(bytes, sizeof(char));
    int size = read(connfd, buf, bytes);
    char req[size];
    strcpy(req, buf);

      // parse the buffer for all of the request information and put it in a request struct
    struct request r = parse_request(req);
    
    struct response rsp = process_request(r);

    pack_response(rsp);

    if (rsp.line.version == NULL) {
        printf("Invalid Request: No version\n");
    }
    // check for errors in the request (wrong version, format, etc) and issue appropriate status
    // send the request to the appropriate method (GET, PUT, APPEND) to deal with the response there

    free(buf);
    //free request
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

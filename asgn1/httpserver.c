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
    char message[100];
    rsp.line.code = error_code;
    switch (error_code) {
        case 200:
            strcpy(message, "OK");
            break;
        case 404:
            strcpy(message, "File Not Found");
            break;
        case 501:
            strcpy(message, "Not Implemented");
            break;
        case 505:
            strcpy(message, "HTTP Version Not Supported");
            break;
    }

    if (rsp.body == NULL) {
        rsp.body = (char *) calloc(1+(int)strlen(message), sizeof(char));
        strcpy(rsp.body, message);
    }
    rsp.line.phrase = (char *) calloc((int)strlen(message), sizeof(char));
    strcpy(rsp.line.phrase, message);
    char nl = '\n';
    strncat(rsp.body, &nl, 1);
    
    rsp.num_headers = 1;
    strcpy(rsp.headers[0].head, "Content-Length");
    sprintf(rsp.headers[0].val, "%lu", strlen(rsp.body));
    return rsp;

}

struct response GET(struct response rsp, struct request req) {
    int bytes;
    for (int i=0; i<req.num_headers; i++) {
        if (strcmp(req.headers[i].head, "Content-Length")==0) {
            printf("String %s vs num %d\n", req.headers[i].val, atoi(req.headers[i].val));
            bytes = atoi(req.headers[i].val)-1;
        }
    }
    printf("BYTES: %d\n", bytes);
    int fd;
    int total = 0;
    int size;
    rsp.body = (char *) calloc(bytes, sizeof(char));
    char *buf = (char *) calloc(bytes, sizeof(char));
    if ((fd = open(req.line.URI, O_RDONLY)) > 0) {
        while ((size = read(fd, buf, bytes)) > 0 && total < bytes) {
            printf("SIZE: %d\n", size);
            strncat(rsp.body, buf, size);
            total += size;
        }
    }
    else {
        return status(rsp, 404);
    }
    printf("%s\n", req.line.URI);
    return status(rsp, 200);
}

struct response PUT(struct response rsp, struct request req) {
    printf("%s\n", req.line.URI);
    return rsp;
}
struct response APPEND(struct response rsp, struct request req) {
    printf("%s\n", req.line.URI);
    return rsp;
}



struct response process_request(struct request req) {
    struct response rsp;
    rsp.line.version = "HTTP/1.1";
    if (strcmp(req.line.version, rsp.line.version) != 0) {
        return status(rsp, 505);
    }

    if (strcmp(req.line.method, "GET")==0) {
        rsp = GET(rsp, req);
    }
    else if (strcmp(req.line.method, "PUT")==0) {
        rsp = PUT(rsp, req);
    }
    else if (strcmp(req.line.method, "APPEND")==0) {
        rsp = APPEND(rsp, req);
    }
    else {
        return status(rsp, 501);
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
    char r[size];
    strcpy(r, buf);

      // parse the buffer for all of the request information and put it in a request struct
    struct request req = parse_request(r);
    
    struct response rsp = process_request(req);

    pack_response(rsp);

    if (rsp.line.version == NULL) {
        printf("Invalid Request: No version\n");
    }
    // check for errors in the request (wrong version, format, etc) and issue appropriate status
    // send the request to the appropriate method (GET, PUT, APPEND) to deal with the response there

    free(buf);
    //free request
    printf("Fix delete functions pls\n");
    delete_request(req);
    delete_response(rsp);
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

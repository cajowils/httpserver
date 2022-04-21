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

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "requests.h"
#include "response.h"

struct response status(struct response rsp, int error_code) {
    char message[100];
    rsp.line.code = error_code;
    int success = 0;
    switch (error_code) {
        case 200:
            strcpy(message, "OK");
            success = 1;
            break;
        case 404:
            strcpy(message, "Not Found");
            break;
        case 501:
            strcpy(message, "Not Implemented");
            break;
        case 505:
            strcpy(message, "HTTP Version Not Supported");
            break;
    }
    printf("MESSAGE: %s\nLEN: %lu\n", message, strlen(message));
    strncpy(rsp.line.phrase, message, (int)strlen(message));

    if (success == 0) {
        rsp.num_headers = 1;
        strcpy(rsp.headers[0].head, "Content-Length");
        sprintf(rsp.headers[0].val, "%lu", strlen(rsp.line.phrase)+1);
    }
    
    return rsp;

}

struct response GET(struct response rsp, struct request req) {
    int bytes = 100000;
    printf("BYTES: %d\n", bytes);
    int fd;
    //int size;
    //rsp.body = (char *) calloc(bytes, sizeof(char));
    //char *buf = (char *) calloc(bytes, sizeof(char));
    if ((fd = open(req.line.URI, O_RDONLY)) > 0) {
        rsp.fd = fd;
        rsp.read = 1;
    }
    else {
        return status(rsp, 404);
    }
    struct stat st;
    fstat(rsp.fd, &st);
    printf("size of file: %ld\n", st.st_size);
    
    strcpy(rsp.headers[rsp.num_headers].head, "Content-Length");
    printf("tst2\n");
    snprintf(rsp.headers[rsp.num_headers].val, 5, "%ld", st.st_size); // +1 because of the ending newline
    rsp.num_headers++;
    printf("%s\n", req.line.URI);
    return status(rsp, 200);
}

struct response PUT(struct response rsp, struct request req) {
    int bytes;
    for (int i=0; i<req.num_headers; i++) {
        if (strcmp(req.headers[i].head, "Content-Length")==0) {
            printf("String %s vs num %d\n", req.headers[i].val, atoi(req.headers[i].val));
            bytes = atoi(req.headers[i].val)-1; //check if val is actually an int, otherwise return 400 bad request
        }
    }
    printf("%s\n", req.line.URI);
    return rsp;
}
struct response APPEND(struct response rsp, struct request req) {
    printf("%s\n", req.line.URI);
    return rsp;
}



struct response process_request(struct request req) {
    struct response rsp = new_response();
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

void send_response(struct response rsp, int connfd) {
    int size;
    size = (int)strlen(rsp.line.version) + (int)strlen(rsp.line.phrase) + 8; //need 3 for the status code, 2 for epaces and 2 for the carriage return
    char buf[size];
    snprintf(buf, size, "%s %d %s\r\n", rsp.line.version, rsp.line.code, rsp.line.phrase);
    write(connfd, buf, size);
    for (int i=0; i < rsp.num_headers; i++) {
        printf("%s: %s\r\n", rsp.headers[i].head, rsp.headers[i].val);
        size = (int)strlen(rsp.headers[i].head) + (int)strlen(rsp.headers[i].val) + 5;
        char buf[size];
        snprintf(buf, size, "%s: %s\r\n", rsp.headers[i].head, rsp.headers[i].val);
        write(connfd, buf, size);
    }
    char *creturn = "\r\n";
    write(connfd, creturn, strlen(creturn));
    if (rsp.read) {
        int size;
        int bytes = 2048;
        char buf[bytes];
        while ((size = read(rsp.fd, buf, bytes)) > 0) {
            write(connfd, buf, size);
        }
        char *nl = "\n";
        write(connfd, nl, 1);
    }
    else {
        size = (int)strlen(rsp.line.phrase) + 2;
        char buf[size];
        snprintf(buf, size, "%s\n", rsp.line.phrase);
        write(connfd, buf, size);
    }
    
    return;
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
    char r[bytes];
    int size = read(connfd, r, bytes);
    printf("Size: %d\n", size);
      // parse the buffer for all of the request information and put it in a request struct
    struct request req = parse_request(r);
    
    struct response rsp = process_request(req);
    printf("TEST2\n");

    send_response(rsp, connfd);

    if (rsp.line.version == NULL) {
        printf("Invalid Request: No version\n");
    }
    // check for errors in the request (wrong version, format, etc) and issue appropriate status
    // send the request to the appropriate method (GET, PUT, APPEND) to deal with the response there

    //free(buf);
    //free request
    delete_request(req);
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

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

struct response status(struct response rsp, int error_code) {
    char message[100];
    rsp.line.code = error_code;
    switch (error_code) {
        case 200:
            strcpy(message, "OK");
            break;
        case 201:
            strcpy(message, "Created");
            break;
        case 400:
            strcpy(message, "Bad Request");
            break;
        case 403:
            strcpy(message, "Forbidden");
            break;
        case 404:
            strcpy(message, "Not Found");
            break;
        case 500:
            strcpy(message, "Internal Server Error");
            break;
        case 501:
            strcpy(message, "Not Implemented");
            break;
        /*case 505:
            strcpy(message, "HTTP Version Not Supported");
            break;*/
    }
    strncpy(rsp.line.phrase, message, (int)strlen(message));

    if (rsp.content_set == 0) {
        strcpy(rsp.headers[rsp.num_headers].head, "Content-Length");
        sprintf(rsp.headers[rsp.num_headers].val, "%lu", strlen(rsp.line.phrase)+1);
        rsp.num_headers++;
        rsp.content_set++;
    }
    
    return rsp;

}

struct response GET(struct response rsp, struct request req) {
    //int size;
    //rsp.body = (char *) calloc(bytes, sizeof(char));
    //char *buf = (char *) calloc(bytes, sizeof(char));
    switch (rsp.fd = open(req.line.URI, O_RDONLY)) {
        case ENOENT:
            return status(rsp, 404);
        case EACCES:
            return status(rsp, 403);
        case EISDIR:
            return status(rsp, 403); //check this code because it may not be correct
    }
    
    rsp.mode = 0;
    struct stat st;
    fstat(rsp.fd, &st);
    
    strcpy(rsp.headers[rsp.num_headers].head, "Content-Length");
    rsp.content_set = 1;
    snprintf(rsp.headers[rsp.num_headers].val, 5, "%ld", st.st_size);
    rsp.num_headers++;
    printf("%s\n", req.line.URI);
    return status(rsp, 200);
}

struct response PUT(struct response rsp, struct request req) {
    rsp.mode = 1;
    uint16_t bytes;
    int s;
    for (int i=0; i<req.num_headers; i++) {
        if (strcmp(req.headers[i].head, "Content-Length")==0) {
            if ((bytes = strtouint16(req.headers[i].val)) == 0) {
                return status(rsp, 400);
            }
        }
    }
    rsp.fd = open(req.line.URI, O_WRONLY | O_TRUNC);
    switch (errno) {
        case ENOENT:
            rsp.fd = open(req.line.URI, O_WRONLY | O_CREAT | O_TRUNC);
            s = 201;
            break;
        case 0:
            s = 200;
            break;
        case EACCES:
            return status(rsp, 403);
        case EISDIR:
            return status(rsp, 403); //check this code because it may not be correct
    }

    if (bytes > req.body_size) {
        bytes = req.body_size;
    }
 
    write(rsp.fd, req.body, bytes);

    return status(rsp, s);
}
struct response APPEND(struct response rsp, struct request req) {
    rsp.mode = 2;
    uint16_t bytes;
    int s;
    for (int i=0; i<req.num_headers; i++) {
        if (strcmp(req.headers[i].head, "Content-Length")==0) {
            if ((bytes = strtouint16(req.headers[i].val)) == 0) {
                return status(rsp, 400);
            }
        }
    }
    
    rsp.fd = open(req.line.URI, O_WRONLY | O_APPEND);

    switch (errno) {
        case 0:
            s = 200;
            break;
        case ENOENT:
            return status(rsp, 404);
        case EACCES:
            return status(rsp, 403);
        case EISDIR:
            return status(rsp, 403); //check this code because it may not be correct
    }
    if (bytes > req.body_size) {
        bytes = req.body_size;
    }
    write(rsp.fd, req.body, bytes);
    return status(rsp, s);
}



struct response process_request(struct request req) {
    struct response rsp = new_response();
    if (strcmp(req.line.version, rsp.line.version) != 0) {
        return status(rsp, 400);
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
    char buf[2048];
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

    if (rsp.mode == 0) {
        int size;
        int bytes = 2048;
        char buf[bytes];
        while ((size = read(rsp.fd, buf, bytes)) > 0) {
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
    char r[bytes];
    read(connfd, r, bytes);
      // parse the buffer for all of the request information and put it in a request struct
    struct request req = parse_request(r);
    
    struct response rsp = process_request(req);

    send_response(rsp, connfd);

    // check for errors in the request (wrong version, format, etc) and issue appropriate status
    // send the request to the appropriate method (GET, PUT, APPEND) to deal with the response there

    delete_request(req);
    close(rsp.fd);
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

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

#include "requests.h"
#include "response.h"
#include "helper.h"


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
    errno = 0;
    rsp.fd = open(req.line.URI, O_RDONLY);
    switch (errno) {
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
    errno = 0;
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
    
    errno = 0;
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
    if (req.error != 0) {
        printf("error\n");
        return status(rsp, req.error);
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


struct response new_response() {
    struct response rsp;
    rsp.line.version = "HTTP/1.1";
    rsp.num_headers = 0;
    rsp.mode = -1;
    rsp.content_set = 0;
    rsp.fd = -1;
    return rsp;
}

void delete_response(struct response rsp) {
    (void) rsp;
    //printf("TEST\n");
    //free(rsp.line.version);
    /*
    for (int h = 0; h < req.num_headers; h++) {
        free(req.headers[h].head);
    }
    */
    return;
}

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
#include "list.h"


struct response status(struct response rsp, int error_code) {
    rsp.line.code = error_code;
    rsp.line.phrase = (char *)calloc(1, sizeof(char)*50);
    switch (error_code) {
        case 200:
            strcpy(rsp.line.phrase, "OK");
            break;
        case 201:
            strcpy(rsp.line.phrase, "Created");
            break;
        case 400:
            strcpy(rsp.line.phrase, "Bad Request");
            break;
        case 403:
            strcpy(rsp.line.phrase, "Forbidden");
            break;
        case 404:
            strcpy(rsp.line.phrase, "Not Found");
            break;
        case 500:
            strcpy(rsp.line.phrase, "Internal Server Error");
            break;
        case 501:
            strcpy(rsp.line.phrase, "Not Implemented");
            break;
        /*case 505:
            strcpy(message, "HTTP Version Not Supported");
            break;*/
    }
    Node *ptr = rsp.headers;
    if (rsp.mode == 0 && rsp.line.code == 200) {
        struct stat st;
        fstat(rsp.fd, &st);
        printf("file size: %ld\n", st.st_size);
        int head_size = (int)strlen("Content-Length");
        int stat_size = (int)st.st_size;
        int val_size = 2; // was 1, but Content-Length of 0 will appear as nothing unless 2 are allocated for some reason
        while (stat_size > 0) {
            stat_size /= 10;
            val_size ++;
        }
        ptr->next = create_node(head_size, val_size);
        ptr = ptr->next;
        strncpy(ptr->head, "Content-Length", head_size);
        snprintf(ptr->val, val_size, "%ld", st.st_size);
        rsp.content_set = 1;
        rsp.num_headers++;
        
    }
    else {

        int head_size = (int)strlen("Content-Length");
        int phrase_size = (int)strlen(rsp.line.phrase) + 1;
        int val_size = 1;
        int phrase_calc = phrase_size;
        while (phrase_calc > 0) {
            phrase_calc /= 10;
            val_size ++;
        }
        
        ptr->next = create_node(head_size, val_size);
        ptr = ptr->next;
        strncpy(ptr->head, "Content-Length", head_size);
        snprintf(ptr->val, val_size, "%d", phrase_size);

        rsp.num_headers++;
        rsp.content_set++;
    }

    return rsp;

}

struct response GET(struct response rsp, struct request req) {
    errno = 0;
    rsp.fd = open(req.line.URI, O_RDONLY);
    printf("error: %d\n", errno);
    switch (errno) {
        case ENOENT:
            return status(rsp, 404);
        case EACCES:
            return status(rsp, 403);
        case EISDIR:
            return status(rsp, 403); //check this code because it may not be correct
    }
    
    return status(rsp, 200);
}

struct response PUT(struct response rsp, struct request req) {
    int s;
    
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

    int bytes = req.body_size;
 
    write(rsp.fd, req.body, bytes);

    return status(rsp, s);
}
struct response APPEND(struct response rsp, struct request req) {
    int s;
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
    int bytes = req.body_size;

    write(rsp.fd, req.body, bytes);

    return status(rsp, s);
}



struct response process_request(struct request req) {
    struct response rsp = new_response();
    if (req.error != 0) {
        return status(rsp, req.error);
    }
    rsp.mode = req.mode;

    switch (rsp.mode) {
        case 0: {
            return(GET(rsp, req));
        }
        case 1: {
            return(PUT(rsp, req));
        }
        case 2: {
            return(APPEND(rsp, req));
        }
    }

    
    //look at method and decide if it is acceptable

    //look at the URI and verify that it exists

    //look at version and verify it is the right one

    //read in the headers

    //read the body if indicated by the method

    return status(rsp, 501);
    
}


struct response new_response() {
    struct response rsp;
    rsp.line.version = "HTTP/1.1";
    rsp.num_headers = 0;
    rsp.mode = -1;
    rsp.content_set = 0;
    rsp.fd = -1;
    rsp.headers = create_node(0,0);
    return rsp;
}

void delete_response(struct response rsp) {
    close(rsp.fd);
    delete_list(rsp.headers);
    free(rsp.line.phrase);
    return;
}

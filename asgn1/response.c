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

int write_all(struct request req, struct response rsp, int fd) {
    if (write(rsp.fd, req.body, req.body_read) < 0) {
        return -1;
    }
    int bytes = 4096;
    int size = 0;
    char *buf = (char *) calloc(1, sizeof(char) * bytes);
    int bytes_written = 0;
    int read_bytes = 0;

    if (req.body_size > req.body_read) {

        do {
            if ((size = read(fd, buf, bytes-1)) < 0) {
                return -1;
            }
            if (req.body_read < req.body_size) {
                read_bytes = (req.body_size - req.body_read > size) ? size : req.body_size - req.body_read;
                if ((bytes_written = write(rsp.fd, buf, read_bytes)) < 0) {
                    return -1;
                }
                req.body_read += bytes_written;
            }
        } while (size > 0);
    }
    free(buf);

    return 0;
}

//takes a rsp and an error code and formats the response appropriately

struct response status(struct response rsp, int error_code) {
    rsp.line.code = error_code;
    rsp.line.phrase = (char *) calloc(1, sizeof(char) * 50);
    switch (error_code) {
    case 200: strcpy(rsp.line.phrase, "OK"); break;
    case 201: strcpy(rsp.line.phrase, "Created"); break;
    case 400: strcpy(rsp.line.phrase, "Bad Request"); break;
    case 403: strcpy(rsp.line.phrase, "Forbidden"); break;
    case 404: strcpy(rsp.line.phrase, "Not Found"); break;
    case 500: strcpy(rsp.line.phrase, "Internal Server Error"); break;
    case 501:
        strcpy(rsp.line.phrase, "Not Implemented");
        break;
        //case 505: strcpy(message, "HTTP Version Not Supported"); break;
    }

    //Adds the headers to an unsuccessful GET request or any other type of request
    if (rsp.mode != 0 || rsp.line.code != 200) {
        Node *ptr = rsp.headers;
        while (ptr->next != NULL) {
            ptr = ptr->next;
        }
        int head_size = (int) strlen("Content-Length");
        int phrase_size = (int) strlen(rsp.line.phrase) + 1;
        int val_size = 1;
        int phrase_calc = phrase_size;
        while (phrase_calc > 0) { //calculates amount of space needed for the Content-Length value
            phrase_calc /= 10;
            val_size++;
        }

        ptr->next = create_node(head_size, val_size);
        ptr = ptr->next;
        strncpy(ptr->head, "Content-Length", head_size);
        snprintf(ptr->val, val_size, "%d", phrase_size);

        rsp.num_headers++;
        rsp.content_set = 1;
    }

    return rsp;
}

//formats the response if it is a GET request
struct response GET(struct response rsp, struct request req) {
    errno = 0;

    rsp.fd = open(req.line.URI, O_RDONLY);

    //checks if it is a directory
    struct stat st;
    fstat(rsp.fd, &st);
    if (S_ISDIR(st.st_mode)) {
        errno = EISDIR;
    }

    switch (errno) {
    case 0: {
        break;
    }
    case ENOENT: {
        return status(rsp, 404);
    }
    case EACCES: {
        return status(rsp, 403);
    }
    case EISDIR: {
        return status(rsp, 403);
    }
    case EBADF: {
        return status(rsp, 404);
    }
    default: {
        return status(rsp, 404);
    }
    }

    //adds headers to GET request
    int head_size = (int) strlen("Content-Length");
    int stat_size = (int) st.st_size;
    int val_size
        = 2; // was 1, but Content-Length of 0 will appear as nothing unless 2 are allocated for some reason
    while (stat_size > 0) {
        stat_size /= 10;
        val_size++;
    }
    Node *ptr = rsp.headers;
    while (ptr->next != NULL) { // gets last header to append to end of linked list
        ptr = ptr->next;
    }
    ptr->next = create_node(head_size, val_size);
    ptr = ptr->next;
    strncpy(ptr->head, "Content-Length", head_size);
    snprintf(ptr->val, val_size, "%d", (int) st.st_size);
    rsp.content_set = 1;
    rsp.num_headers++;

    return status(rsp, 200);
}

//formats the response if it is a PUT request
struct response
    PUT(struct response rsp, struct request req) {
    int s;
    errno = 0;

    if (access(req.line.URI, F_OK) == 0) {
        rsp.fd = open(req.line.URI, O_WRONLY | O_TRUNC);
        s = 200;
    } else {
        errno = 0;
        rsp.fd = open(req.line.URI, O_WRONLY | O_CREAT | O_TRUNC);
        s = 201;
    }

    struct stat st;
    fstat(rsp.fd, &st);
    if (S_ISDIR(st.st_mode)) {
        errno = EISDIR;
    }

    switch (errno) {
    case 0: {
        break;
    }
    case EACCES: {
        return status(rsp, 403);
    }
    case EISDIR: {
        return status(rsp, 403);
    }
    default: {
        return status(rsp, 403);
        warnx("PUT error");
    }
    }
    //flushes the body that was read in with the request
    if (write_all(req, rsp, req.connfd) < 0) {
        return status(rsp, 500);
    }

    return status(rsp, s);
}

//formats the response if it is an APPEND request
struct response
    APPEND(struct response rsp, struct request req) {

    errno = 0;
    rsp.fd = open(req.line.URI, O_WRONLY | O_APPEND);
    switch (errno) {
    case 0: {
        break;
    }
    case ENOENT: {
        return status(rsp, 404);
    }
    case EACCES: {
        return status(rsp, 403);
    }
    case EISDIR: {
        return status(rsp, 403);
    }
    default: {
        return status(rsp, 404);
    }
    }

    //flushes the body that was read in with the request

    if (write_all(req, rsp, req.connfd) < 0) {
        return status(rsp, 500);
    }

    return status(rsp, 200);
}

//processes the request by sending it to proper method
struct response
    process_request(struct request req) {
    struct response rsp = new_response();
    if (req.error != 0) {
        return status(rsp, req.error);
    }
    rsp.mode = req.mode;

    switch (rsp.mode) {
    case 0: {
        return (GET(rsp, req));
    }
    case 1: {
        return (PUT(rsp, req));
    }
    case 2: {
        return (APPEND(rsp, req));
    }
    }

    return status(rsp, 501);
}

struct response new_response() {
    struct response rsp;
    rsp.line.version = "HTTP/1.1";
    rsp.num_headers = 0;
    rsp.mode = -1;
    rsp.content_set = 0;
    rsp.fd = -1;
    rsp.headers = create_node(0, 0);
    return rsp;
}

void delete_response(struct response rsp) {
    if (rsp.fd != -1) {
        close(rsp.fd);
    }
    delete_list(rsp.headers);
    free(rsp.line.phrase);
    return;
}

#ifndef __REQUESTS_H__
#define __REQUESTS_H__

#include "list.h"

struct request_line {
    char *method;
    char *URI;
    char *version;
};

struct request {
    struct request_line line;
    Node *headers;
    char *body;
    int body_read;
    int num_headers;
    int body_size;
    int mode;
    int error;
    int connfd;
};

int read_all(int fd, char *buf, int nbytes);

struct request parse_request_regex(int connfd);

struct request new_request();

void delete_request(struct request req);

#endif

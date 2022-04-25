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
};

struct request parse_request_regex(char *r);

struct request parse_request(char *req);

struct request new_request();

void delete_request(struct request req);

#endif

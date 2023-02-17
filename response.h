#ifndef __RESPONSE_H__
#define __RESPONSE_H__

#include "node.h"

struct response_line {
    char *version;
    int code;
    char *phrase;
};

struct response {
    struct response_line line;
    Node *headers;
    int fd;
    int num_headers;
    int mode;
    int content_set;
    int flushed;
};

struct response status(struct response rsp, int error_code);

struct response GET(struct response rsp, struct request req);

struct response PUT(struct response rsp, struct request req);

struct response APPEND(struct response rsp, struct request req);

struct response process_request(struct request req);

struct response new_response();

void delete_response(struct response rsp);

#endif

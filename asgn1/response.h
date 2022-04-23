#ifndef __RESPONSE_H__
#define __RESPONSE_H__

struct response_line {
    char *version;
    int code;
    char phrase[100];
};

struct rsp_header {
    char head[1024];
    char val[1024];
};

struct response {
    struct response_line line;
    struct rsp_header headers[100];
    int fd;
    int num_headers;
    int mode;
    int content_set;
};

struct response status(struct response rsp, int error_code);

struct response GET(struct response rsp, struct request req);

struct response PUT(struct response rsp, struct request req);

struct response APPEND(struct response rsp, struct request req);

struct response process_request(struct request req);

struct response new_response();

void delete_response(struct response rsp);

#endif

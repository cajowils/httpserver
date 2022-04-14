#ifndef __RESPONSE_H__
#define __RESPONSE_H__

struct response_line {
    char *version;
    int code;
    char *phrase;
};

struct rsp_header {
    char head[1048];
    char val[1048];
};

struct response {
    struct response_line line;
    struct rsp_header headers[100];
    char *body;
    int num_headers;
};

char *pack_response(struct response rsp);

#endif

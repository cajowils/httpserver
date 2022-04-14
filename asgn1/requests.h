#ifndef __REQUESTS_H__
#define __REQUESTS_H__

struct request_line {
    char *method;
    char *URI;
    char *version;
};

struct header {
    char head[1048];
    char val[1048];
};

struct request {
    struct request_line line;
    struct header headers[100];
    char *body;
    int num_headers;
};

struct request parse_request(char *req);

#endif

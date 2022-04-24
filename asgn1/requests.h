#ifndef __REQUESTS_H__
#define __REQUESTS_H__

struct request_line {
    char method[10];
    char URI[20];
    char version[10];
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
    int body_size;
    int mode;
    int error;
};

struct request parse_request_regex(char *r);

struct request parse_request(char *req);

struct request new_request();

void delete_request(struct request req);

#endif

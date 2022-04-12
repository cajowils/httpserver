#ifndef __REQUESTS_H__
#define __REQUESTS_H__

struct request_line {
    char *method;
    char *URI;
    char *version;
};

struct request {
    struct request_line line;
    char **headers;
    char *body;
};

//struct parse_request_line(char* req);

struct request process_request(char *req);

#endif

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

struct request process_request(char *buffer);

#endif

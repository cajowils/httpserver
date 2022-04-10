#include <stdio.h>
#include "requests.h"


struct request process_request(char *buffer) {
    printf("Read: %s", buffer);
    struct request r;
    r.line.method = "GET";
    r.headers = NULL;
    r.body = NULL;
    return r;
}

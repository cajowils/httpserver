#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "response.h"


struct response new_response() {
    struct response rsp;
    rsp.line.version = "HTTP/1.1";
    rsp.num_headers = 0;
    rsp.read = 0;
    return rsp;
}

void delete_response(struct response rsp) {
    (void) rsp;
    //printf("TEST\n");
    //free(rsp.line.version);
    /*
    for (int h = 0; h < req.num_headers; h++) {
        free(req.headers[h].head);
    }
    */
    return;
}

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "response.h"

char *pack_response(struct response rsp) {
    //printf("%s %d %s\r\n%s: %s\r\n\r\n", rsp.line.version, rsp.line.code, rsp.line.phrase, rsp.headers[0].head, rsp.headers[0].val);
    printf("%s %d %s\r\n", rsp.line.version, rsp.line.code, rsp.line.phrase);
    for (int i = 0; i < rsp.num_headers; i++) {
        printf("%s: %s\r\n", rsp.headers[i].head, rsp.headers[i].val);
    }
    printf("\r\n");
    if (rsp.body != NULL) {
        printf("%s", rsp.body);
    }
    
    return "OK";
}

void delete_response(struct response rsp) {
    free(rsp.line.phrase);
    //printf("TEST\n");
    //free(rsp.line.version);
    /*
    for (int h = 0; h < req.num_headers; h++) {
        free(req.headers[h].head);
    }
    */
    free(rsp.body);
    return;
}

#include <stdio.h>
#include <string.h>
#include "response.h"

char *pack_response(struct response rsp) {
    printf("%s %d %s\r\n%s: %s\r\n\r\n", rsp.line.version, rsp.line.code, rsp.line.phrase, rsp.headers[0].head, rsp.headers[0].val);
    return "OK";
}

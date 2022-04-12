#include <stdio.h>
#include <string.h>
#include <regex.h>
#include <stdlib.h>
#include "requests.h"


/*struct parse_request_line(char* req) {
    
}*/



struct request process_request(char *req) {

    struct request r;
    
    r.line.method = (char *) calloc(1, sizeof(char));
    r.line.URI = (char *) calloc(1, sizeof(char));

    char buf[2048];
    int mode = 0;
    int s = 0;

    printf("Request: %s\n", req);
    for (int i = 0; i < (int)strlen(req); i++) {
        if (mode==0) { //dealing with the method
            if (req[i] == ' ') {
                
                strncpy(r.line.method, buf, s);
                mode++;
                s = 0;
            }
            else {
                buf[s] = (char)req[i];
                s++;
            }
        }
        else if (mode==1) {
            if (req[i] == ' ') {
                strncpy(r.line.URI, buf, s);
                mode++;
                s = 0;
            }
            else {
                buf[s] = (char)req[i];
                s++;
            }
        }
        else if (mode==2) {
            //printf("MODE 2\n");
            continue;
        }
        

    }
    


    printf("Method\t>%s<\n", r.line.method);
    printf("URI\t>%s<\n", r.line.URI);
 


    return r;
}

/*char **parse(char *str) {
    
}*/

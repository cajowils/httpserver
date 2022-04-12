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
    r.line.version = (char *) calloc(1, sizeof(char));
    r.body = (char *) calloc(1, sizeof(char));
    //r.headers =  calloc(1, sizeof(char*));
    char buf[2048];
    int mode = 0;
    int s = 0; //iterater for the current buf
    int h = 0; //iterater for the current header
    char prev = ' ';
    int stop_headers = 0; //indicates when it is time to stop reading headers

    printf("Request:\n%s\n", req);
    for (int i = 0; i < (int)strlen(req); i++) {
        //printf("%c\n", req[i]);
        if (mode==0) { //dealing with the method
            if (req[i] == ' ') {
                strncpy(r.line.method, buf, s);
                printf("Method\t>%s<\n", r.line.method);
                mode++;
                s = 0;
            }
            else {
                buf[s] = req[i];
                s++;
            }
        }
        else if (mode==1) {
            if (req[i] == ' ') {
                strncpy(r.line.URI, buf, s);
                printf("URI\t>%s<\n", r.line.URI);
                mode++;
                s = 0;
            }
            else {
                buf[s] = req[i];
                s++;
            }
        }
        else if (mode==2) {
            if (prev == '\r' && req[i] == '\n') {
                strncpy(r.line.version, buf, s);
                printf("Version\t>%s<\n", r.line.version);
                mode ++;
                s = 0;
            }
            else if (req[i] != '\r') {
                buf[s] = req[i];
                s++;
            }
        }
        else if (mode==3) {
            //printf("Char: %c\n", req[i]);
            if (prev == '\r' && req[i] == '\n') {
                if (stop_headers) {
                    mode ++;
                }
                else {
                    r.headers[h] = (char *) calloc(1, sizeof(char));
                    strncpy(r.headers[h], buf, s);
                    //printf("Testing: %s\n", r.headers[h]);
                    h++;
                    stop_headers = 1;
                }
                s = 0;
                
            }
            else if (req[i] != '\r') {
                buf[s] = req[i];
                s++;
                stop_headers = 0;
            }
        }
        else if (mode==4) {
            buf[s] = req[i];
            s++;
        }
        
    prev = req[i];
    }

    if (s > 0) {
        strncpy(r.body, buf, s); //maybe set it to ignore end of file \n ?
    }
    
    int size = (int) (sizeof(r.headers)/sizeof(r.headers[0]));

    printf("Body:\t>%s<\n", r.body);
    
    
    printf("Headers:\n");
    for (int q=0;q<size; q++) {
        printf("\t>%s<\n", r.headers[q]);
    }
 


    return r;
}

/*char **parse(char *str) {
    
}*/

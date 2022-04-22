#include <stdio.h>
#include <string.h>
#include <regex.h>
#include <stdlib.h>
#include <ctype.h>
#include "requests.h"

struct request parse_request(char *req) {

    struct request r = new_request();
    
    /*r.line.method = (char *) calloc(1, sizeof(char));
    r.line.URI = (char *) calloc(1, sizeof(char));
    r.line.version = (char *) calloc(1, sizeof(char));*/
    
    char buf[2048];
    int mode = 0;
    int s = 0; //iterater for the current buf
    int h = 0; //iterater for the current header
    char prev = ' ';
    int stop_headers = 0; //indicates when it is time to stop reading headers
    int before_colon = 1; // indicates what part of the header is being read
    r.num_headers = 0;

    printf("Request:\n%s\n", req);
    for (int i = 0; i < (int)strlen(req)-1; i++) { //len(req)-1 because it cuts off the EOF at the end, preventing it from going into the body of the request
        if (mode==0) { //finding the method
            if (req[i] == ' ') {
                strncpy(r.line.method, buf, s);
                printf("Method\t>%s<\n", r.line.method);
                mode++;
                s = 0;
                i++; //this skips the initial '/' of the URI
            }
            else {
                buf[s] = toupper(req[i]);
                s++;
            }
        }
        else if (mode==1) { //finding the URI
            printf("char: %c\n", req[i]);
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
        else if (mode==2) { //finding the version
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
        else if (mode==3) { //finding the headers
            if (prev == '\r' && req[i] == '\n') {
                if (stop_headers) {
                    mode ++;
                }
                else {
                    strncpy(r.headers[h].val, buf, s);
                    h++;
                    stop_headers = 1;
                    before_colon = 1;
                    if (s > 0) {
                        r.num_headers++;
                    }
                }
                s = 0;
                
            }
            else if (req[i] != '\r') {
                if (req[i] == ':') {
                    strncpy(r.headers[h].head, buf, s);
                    before_colon = 0;
                    s=0;
                    i++; // this skips the space and gets the value of the header
                }
                else {
                    buf[s] = req[i];
                    stop_headers = 0;
                    s++;
                }
            }
        }
        else if (mode==4) { //finding the body of the request
            buf[s] = req[i];
            s++;
        }
    prev = req[i];
    }

    if (s > 0) {
        strncpy(r.body, buf, s); //maybe set it to ignore end of file \n ?
    }
    
    printf("Headers:\n");
    for (int q=0;q<r.num_headers; q++) {
        printf("\t>%s: %s<\n", r.headers[q].head, r.headers[q].val);
    }
 
    printf("Body:\t>%s<\n", r.body);

    return r;
}

struct request new_request() {
    struct request req;
    req.body = (char *) calloc(1, sizeof(char));
    req.num_headers = 0;
    return req;    
}

void delete_request(struct request req) {
    /*
    for (int h = 0; h < req.num_headers; h++) {
        free(req.headers[h].head);
    }
    */
    free(req.body);
    return;
}

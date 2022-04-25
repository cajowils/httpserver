#include <stdio.h>
#include <string.h>
#include <regex.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include "requests.h"
#include "helper.h"
#include "list.h"

struct request parse_request_regex(char *r) {
    struct request req = new_request();

    const char* pattern = "^([a-zA-Z]+) \\/((\\/?[A-Za-z0-9_\\-\\.]+)+) (HTTP\\/[0-9]\\.[0-9])\r\n(([a-zA-Z0-9_\\.-]+: [a-zA-Z0-9_\\.:\\/\\*-]+\r\n)*)\r\n";
    //const char* pattern = "^([a-zA-Z]+) \\/((\\/?[A-Za-z0-9_\\-\\.]+)+) (HTTP\\/[0-9]\\.[0-9])\r\n(([a-zA-Z\\-_\\.]+: [a-zA-Z0-9\\-_\\.:]+\r\n)*)\r\n";
    //const char* method_pattern = "(GET|PUT|APPEND)";

    regex_t re;
    if (regcomp(&re, pattern, REG_EXTENDED) != 0) {
        req.error = 500;
        return req;
    }

    int num_groups = 6;
    regmatch_t groups[num_groups];

    /* 
    GROUPING:

    GET /tests/foo.txt HTTP/1.1
    Content-Length: 50

    0: Whole match (as above)
    1: Method (GET)
    2: Whole URI without leading "/" (tests/foo.txt)
    3: Last part of URI (/foo.txt)
    4: HTTP version (HTTP/1.1)
    5: Headers (Content-Length: 50)

    */

    int headers_start;
    int headers_end;
    int no_headers = 0;

    int result = regexec(&re, r, num_groups, groups, 0);
    regfree(&re);
    if (result == 0) {
        for (int i = 0; i < num_groups; i++) {
            if (groups[i].rm_so == -1) {
                if (i < 5) {
                    req.error = 400;
                    return req;
                }
                else {
                    no_headers = 1;
                    break;
                }
            }

            
            switch (i) {
                case 1: {
                    int size = groups[i].rm_eo - groups[i].rm_so;
                    req.line.method = (char *) calloc(1, sizeof(char)*size);
                    strncpy(req.line.method, r + groups[i].rm_so, size);

                    for (int s = 0; s < size; s++) {
                        req.line.method[s] = toupper(req.line.method[s]);
                    }
                    
                    if (strncmp(req.line.method, "GET", size) == 0) {
                        req.mode = 0;
                    }
                    else if (strncmp(req.line.method, "PUT", size) == 0) {
                        req.mode = 1;
                    }
                    else if (strncmp(req.line.method, "APPEND", size) == 0) {
                        req.mode = 2;
                    }
                    else {
                        req.error = 501;
                        return req;
                    }
                    break;
                }
                case 2: {
                    int size = groups[i].rm_eo - groups[i].rm_so;
                    req.line.URI = (char *) calloc(1, sizeof(char)*size);
                    strncpy(req.line.URI, r + groups[i].rm_so, size);
                    break;
                }
                case 4: {
                    int size = groups[i].rm_eo - groups[i].rm_so;
                    req.line.version = (char *) calloc(1, sizeof(char)*size);
                    strncpy(req.line.version, r + groups[i].rm_so, size);

                    headers_start = groups[i].rm_eo;

                    if (strncmp(req.line.version, "HTTP/1.1", size) != 0) {
                        req.error = 400;
                        return req;
                    }
                    break;
                }
                case 5: {
                    headers_end = groups[i].rm_eo;
                    //printf("headers end : %d\n", headers_end);
                    break;
                }

            }
            

        }
    }
    else {
        
        req.error = 400;
        return req;
    }
    int content_found = 0;

    if (!no_headers) {
        int h_size = headers_end - headers_start;
        char *headers = (char *) calloc(1, sizeof(char) * h_size);
        strncpy(headers, r + headers_start, h_size);

        char *h_pattern = "([a-zA-Z0-9-]+): ([a-zA-Z0-9_\\.:\\/\\*-]+)\r\n";
        //char *h_pattern = "(HTTP\\/1\\.1)\r\n(([a-zA-Z\\-_\\.]+: [a-zA-Z0-9\\-_\\.:\\/\\*]+\r\n)*)\r\n";
        //char *h_pattern = "\r\n([a-zA-Z0-9_\\.-]+: [a-zA-Z0-9_\\.:\\/\\*-]+\r\n)*\r\n";

        regex_t h_re;
        if (regcomp(&h_re, h_pattern, REG_EXTENDED) != 0) {
                free(headers);
                regfree(&h_re);
                req.error = 500;
                return req;
            }
        
        int num_h_matches = 5;
        int num_h_groups = 3;
        regmatch_t h_groups[num_h_groups];
        Node *ptr = req.headers;
        char *curr_header = headers;

        for (int m=0; m < num_h_matches; m++) {
            int h_result = regexec(&h_re, curr_header, num_h_groups, h_groups, 0);
            int offset = 0;

            if (h_result) {break;}
            offset = h_groups[0].rm_eo;
            

            for (int i = 0; i < num_h_groups; i++) {
                if (h_groups[i].rm_so == -1) {
                    if (i < 3) {
                        free(headers);
                        regfree(&h_re);
                        req.error = 400;
                        return req;
                    }
                    break;
                }
                
            }

            int head_size = h_groups[1].rm_eo - h_groups[1].rm_so;
            int val_size = h_groups[2].rm_eo - h_groups[2].rm_so;
            ptr->next = create_node(head_size, val_size);
            ptr = ptr->next;
            strncpy(ptr->head, curr_header + h_groups[1].rm_so, head_size);
            strncpy(ptr->val, curr_header + h_groups[2].rm_so, val_size);
            req.num_headers++;

            if (strcmp(ptr->head, "Content-Length") == 0 && !content_found) {
                content_found = 1;
                int val = strtoint(ptr->val);
                if (val > 0) {
                    req.body_size = val;
                }
            }

            curr_header += offset;
        }

        free(headers);
        regfree(&h_re);
    }
 
    if (req.mode == 1 || req.mode == 2) {
        if (!content_found) {  // need content-length header for PUT and APPEND requests
            req.error = 400;
            return req;
        }

        
        int body_start = headers_end + 2;

        req.body_read = (int)strlen(r) - body_start;

        req.body_read = (req.body_read > req.body_size) ? req.body_size : req.body_read;
        strncpy(req.body, r + body_start, req.body_read);
    }

    

    return req;
}
/*
struct request parse_request(char *req) {

    //char *re = "^([a-zA-Z]+) \\/((?:\\/?[A-Za-z0-9_\\-\\.]+)+) (HTTP\\/[0-9]\\.[0-9])";

    struct request r = new_request();
    
    char buf[2048];
    int mode = 0;
    int s = 0; //iterater for the current buf
    int h = 0; //iterater for the current header
    char prev = ' ';
    int stop_headers = 0; //indicates when it is time to stop reading headers
    int before_colon = 1; // indicates what part of the header is being read
    r.num_headers = 0;

    printf("Request:\n%s\n", req);
    for (int i = 0; i < (int)strlen(req); i++) { //len(req)-1 because it cuts off the EOF at the end, preventing it from going into the body of the request
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
                if (req[i] == ':' && before_colon) {
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
        r.body_size = s;
        strncpy(r.body, buf, s);
    }
    
    printf("Headers:\n");
    for (int q=0;q<r.num_headers; q++) {
        printf("\t>%s: %s<\n", r.headers[q].head, r.headers[q].val);
    }
 
    printf("Body:\t>%s<\n", r.body);

    return r;
}
*/

struct request new_request() {
    struct request req;
    req.body = (char *) calloc(1, sizeof(char) * 4096);
    req.body_read = 0;
    req.num_headers = 0;
    req.body_size = 0;
    req.error = 0;
    req.headers = create_node(0,0);
    return req;    
}

void delete_request(struct request req) {
    delete_list(req.headers);
    free(req.body);
    free(req.line.method);
    free(req.line.URI);
    free(req.line.version);
    return;
}

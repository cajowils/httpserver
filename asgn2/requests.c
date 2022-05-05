#include <stdio.h>
#include <string.h>
#include <regex.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include "requests.h"
#include "helper.h"
#include "list.h"

struct request parse_request_regex(char *r, int size) {
    struct request req = new_request();

    if (size < 0) {
        req.error = 500;
        return req;
    }

    if (size < 1) {
        req.error = 400;
        return req;
    }

    const char *pattern
        = "^([a-zA-Z]+) \\/((\\/?[A-Za-z0-9_\\-\\.]+)+) "
          "(HTTP\\/[0-9]\\.[0-9])\r\n(([a-zA-Z0-9_\\.-]+: [a-zA-Z0-9_\\.:\\/\\*-]+\r\n)*)\r\n";

    regex_t re;
    if (regcomp(&re, pattern, REG_EXTENDED) != 0) {
        req.error = 500;
        return req;
    }

    int num_groups = 6;
    regmatch_t groups[num_groups];

    /* 
    GROUPING ON MATCH:

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
    int body_start;

    int result = regexec(&re, r, num_groups, groups, 0);
    regfree(&re);
    if (result == 0) {
        for (int i = 0; i < num_groups; i++) {
            if (groups[i].rm_so == -1) {
                num_groups = i;
                if (i < 5) {
                    req.error = 400;
                    return req;
                } else {
                    no_headers = 1;
                    break;
                }
            }
        }

        // Getting the index of the body
        body_start = groups[0].rm_eo;

        // Getting Method
        int method_size = groups[1].rm_eo - groups[1].rm_so;
        req.line.method = (char *) calloc(1, sizeof(char) * method_size);
        strncpy(req.line.method, r + groups[1].rm_so, method_size);

        if (strncmp(req.line.method, "GET", method_size) == 0
            || strncmp(req.line.method, "get", method_size) == 0) {
            req.mode = 0;
        } else if (strncmp(req.line.method, "PUT", method_size) == 0
                   || strncmp(req.line.method, "put", method_size) == 0) {
            req.mode = 1;
        } else if (strncmp(req.line.method, "APPEND", method_size) == 0
                   || strncmp(req.line.method, "append", method_size) == 0) {
            req.mode = 2;
        } else {
            req.error = 501;
            return req;
        }

        //Getting URI
        int uri_size = groups[2].rm_eo - groups[2].rm_so;
        req.line.URI = (char *) calloc(1, sizeof(char) * uri_size + 1);
        strncpy(req.line.URI, r + groups[2].rm_so, uri_size);

        //Gettng HTTP Version
        int version_size = groups[4].rm_eo - groups[4].rm_so;
        req.line.version = (char *) calloc(1, sizeof(char) * version_size + 1);
        strncpy(req.line.version, r + groups[4].rm_so, version_size);
        headers_start = groups[4].rm_eo;

        if (strncmp(req.line.version, "HTTP/1.1", version_size) != 0) {
            req.error = 400;
            return req;
        }

        //Getting the end of the headers
        if (num_groups > 4) {
            headers_end = groups[5].rm_eo;
        }

    } else {

        req.error = 400;
        return req;
    }

    int content_found = 0;

    if (!no_headers) {

        //Matches the headers and stores them in a linked list

        int h_size = headers_end - headers_start;
        char *headers = (char *) calloc(1, sizeof(char) * h_size + 1);
        strncpy(headers, r + headers_start, h_size);

        char *h_pattern = "([a-zA-Z0-9-]+): ([a-zA-Z0-9_\\.:\\/\\*-]+)\r\n";

        regex_t h_re;
        if (regcomp(&h_re, h_pattern, REG_EXTENDED) != 0) {
            free(headers);
            regfree(&h_re);
            req.error = 500;
            return req;
        }

        int num_h_matches = 1000;
        int num_h_groups = 3;
        regmatch_t h_groups[num_h_groups];

        Node *ptr = req.headers;
        char *curr_header = headers;

        for (int m = 0; m < num_h_matches; m++) {
            int h_result = regexec(&h_re, curr_header, num_h_groups, h_groups, 0);
            int offset = 0;

            if (h_result) {
                break;
            }
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

            // Copy the header to the linked list node

            int head_size = h_groups[1].rm_eo - h_groups[1].rm_so;
            int val_size = h_groups[2].rm_eo - h_groups[2].rm_so;
            ptr->next = create_node(head_size, val_size);
            ptr = ptr->next;
            strncpy(ptr->head, curr_header + h_groups[1].rm_so, head_size);
            strncpy(ptr->val, curr_header + h_groups[2].rm_so, val_size);
            req.num_headers++;

            if (strcmp(ptr->head, "Content-Length") == 0
                && !content_found) { // Checks for Content-Length
                if (is_number(ptr->val, val_size)) {
                    content_found = 1;
                    int val = strtoint(ptr->val);
                    if (val > 0) {
                        req.body_size = val;
                    }
                }
            }

            if (strcmp(ptr->head, "Request-ID") == 0) {
                if (is_number(ptr->val, val_size)) {
                    int ID = strtoint(ptr->val);
                    if (ID > 0) {
                        req.ID = ID;
                    }
                }
                else {
                    req.error = 400;
                    free(headers);
                    regfree(&h_re);
                    return req;
                }
            }

            curr_header += offset;
        }

        free(headers);
        regfree(&h_re);
    }
    if (req.mode == 1 || req.mode == 2) {
        if (!content_found) { // need content-length header for PUT and APPEND requests
            req.error = 400;
            return req;
        }
    
        req.body_read = size - body_start;

        req.body_read = (req.body_read > req.body_size) ? req.body_size : req.body_read;
        strncpy(req.body, r + body_start, req.body_read);
    }

    return req;
}

struct request new_request() {
    struct request req;
    req.line.method = NULL;
    req.line.URI = NULL;
    req.line.version = NULL;
    req.body = (char *) calloc(1, sizeof(char) * 4096);
    req.body_read = 0;
    req.num_headers = 0;
    req.body_size = 0;
    req.error = 0;
    req.headers = create_node(0, 0);
    req.ID = 0;
    return req;
}

void delete_request(struct request req) {
    delete_list(req.headers);
    free(req.body);
    if (req.line.method) {
        free(req.line.method);
    }
    if (req.line.URI) {
        free(req.line.URI);
    }
    if (req.line.version) {
        free(req.line.version);
    }
    return;
}

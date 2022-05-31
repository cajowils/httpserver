#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/file.h>
#include <unistd.h>
#include <err.h>

#include "list.h"
#include "requests.h"
#include "response.h"

//QueueNode

//Stores connections so that they can be stored and processed later

//buf: a place to store the request as its being read in
//val: the fd of the connection
//size: the length of the request read in so far
//next: pointer to the next node in the queue/list
//request: indicates whether the request is ready to be parsed
//flushed: indicates whether the body that was read in with the request has already been flushed to the file
//also keeps other information captured by request and response structs that are useful for storing later

QueueNode *create_queue_node(int val) {
    QueueNode *qn = (QueueNode *) malloc(sizeof(QueueNode));
    qn->buf = (char *) malloc(sizeof(char) * 4096);
    memset(qn->buf, '\0', 4096);
    qn->val = val;
    qn->size = 0;
    qn->next = NULL;
    qn->request = 0;
    qn->op = -1;
    qn->tmp = -1;
    qn->tmp_name = (char *) malloc(sizeof(char) * 11);
    memset(qn->tmp_name, '\0', 11);
    memcpy(qn->tmp_name, "tmp_XXXXXX", 10);
    return qn;
}

void delete_queue_node(QueueNode *qn) {
    if (qn) {
        if (qn->rsp.fd >= 0) {
            close(qn->rsp.fd);
        }
        if (qn->val >= 0) {
            close(qn->val);
        }
        if (qn->tmp >= 0) {
            close(qn->tmp);
        }
        delete_request(qn->req);
        delete_response(qn->rsp);
        qn->next = NULL;
        free(qn->buf);
        qn->buf = NULL;
        free(qn->tmp_name);
        qn->tmp_name = NULL;
        free(qn);
        qn = NULL;
    }
    return;
}

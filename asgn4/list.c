#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/file.h>
#include <unistd.h>

#include "list.h"

//Linked List

///Roughly based off of Eugene's section code from my CSE13s Fall 2020 class

//head: the first value of the header ("Content-Length")
//val: the second value of the header ("100")
//next: the next node of the list

//starts with a dummy node as the head of the list

Node *create_node(int head, int val) {
    Node *n = (Node *) malloc(sizeof(Node));
    n->next = NULL;
    n->head = (char *) malloc(sizeof(char) * head + 1);
    memset(n->head, '\0', head + 1);
    n->val = (char *) malloc(sizeof(char) * val + 1);
    memset(n->val, '\0', val + 1);
    return n;
}

void delete_node(Node *n) {
    free(n->head);
    free(n->val);
    free(n);
    n = NULL;
    return;
}

void delete_list(Node *head) {
    if (head != NULL) {
        Node *ptr = head;
        Node *next;
        do {
            next = ptr->next;
            delete_node(ptr);
            ptr = next;
        } while (next != NULL);
    }
}

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
    qn->flushed = 0;
    return qn;
}

void delete_queue_node(QueueNode *qn) {
    if (qn) {
        if (qn->req_fd >= 0) {
            close(qn->req_fd);
        }
        if (qn->val >= 0) {
            close(qn->val);
        }
        qn->next = NULL;
        free(qn->buf);
        qn->buf = NULL;
        free(qn);
        qn = NULL;
    }
    return;
}

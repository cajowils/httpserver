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
    Node *n = (Node *) calloc(1, sizeof(Node));
    n->next = NULL;
    n->head = (char *) calloc(1, sizeof(char) * head);
    n->val = (char *) calloc(1, sizeof(char) * val);
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

QueueNode *create_queue_node(int val) {
    QueueNode *qn = (QueueNode *) malloc(sizeof(QueueNode));
    qn->buf = (char *) malloc(sizeof(char) * 2048);
    memset(qn->buf, '\0', 2048);
    qn->val = val;
    qn->size = 0;
    qn->next = NULL;
    return qn;
}

void delete_queue_node(QueueNode *qn) {
    if (qn) {
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

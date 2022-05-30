#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "node.h"

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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "list.h"


Node *create_node(int head, int val) {
    Node *n = (Node *)calloc(1, sizeof(Node));
    n->next = NULL;
    n->head = (char *)calloc(1, sizeof(char)*head);
    n->val = (char *)calloc(1, sizeof(char)*val);
    return n;
}

void delete_node(Node *n) {
    //printf("deleting header: %s: %s\n", n->head, n->val);
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
        //printf("Starting to delete\n");
        do {
            next = ptr->next;
            delete_node(ptr);
            ptr = next;
        }
        while(next != NULL);
    }
}

#ifndef __LIST_H__
#define __LIST_H__

typedef struct Node Node;

struct Node {
    char *head;
    char *val;
    Node *next;
};

Node *create_node(int head, int val);

void delete_node(Node *n);

void delete_list(Node *head);

#endif

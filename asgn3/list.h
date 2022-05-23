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

typedef struct QueueNode QueueNode;

struct QueueNode {
    int val;
    int size;
    char *buf;
    QueueNode *next;
};

QueueNode *create_queue_node(int val);

void delete_queue_node(QueueNode *qn);

#endif

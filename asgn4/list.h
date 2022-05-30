#ifndef __LIST_H__
#define __LIST_H__

#include "requests.h"
#include "response.h"

typedef struct QueueNode QueueNode;

struct QueueNode {
    int val;
    int size;
    char *buf;
    int request;
    int op;
    char *tmp_name;
    int tmp;
    struct request req;
    struct response rsp;
    QueueNode *next;
};

QueueNode *create_queue_node(int val);

void delete_queue_node(QueueNode *qn);

#endif

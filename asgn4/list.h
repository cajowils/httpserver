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
    struct request req;
    struct response rsp;
    int flushed;
    QueueNode *next;
};

QueueNode *create_queue_node(int val);

void delete_queue_node(QueueNode *qn);


typedef struct DictNode DictNode;

struct DictNode {
    char key[32];
    void *value;
    DictNode *next;
    DictNode *prev;
};

typedef struct DictList DictList;

struct DictList {
    DictNode *head;
};

DictNode *find_dict_list(DictList *list, char *key);

int insert_dict_list(DictList *list, DictNode *node);

int delete_dict_list(DictList *list, DictNode *node);

void print_dict_list(DictList *list);




#endif

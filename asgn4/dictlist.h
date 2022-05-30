#ifndef __DICTLIST_H__
#define __DICTLIST_H__

#include "queue.h"

typedef struct DictNode DictNode;

struct DictNode {
    char key[32];
    int writing;
    int readers;
    Queue *queue;
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

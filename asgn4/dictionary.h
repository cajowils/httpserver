#ifndef __DICTIONARY_H__
#define __DICTIONARY_H__

#include "list.h"
#include "queue.h"

typedef struct element element;

struct element {
    char key[32];
    Queue queue;
};

typedef struct dictionary dictionary;

struct dictionary {
    int        slots;
    int        size;
    DictList     *hash_table;
};  

dictionary *create(int slots);

int insert(dictionary *D, char *key);

int del(dictionary *D, char *key);

DictNode *find_dict(dictionary *D, char *key);

int hash(char key[32], int slots);

int destruct(dictionary* D);

void print_dict(dictionary* D);


#endif

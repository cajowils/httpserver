#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <stdio.h>

#include "dictionary.h"
#include "dictlist.h"
#include "queue.h"

dictionary *create(int slots) {
    dictionary *D = malloc(sizeof(dictionary));
    D->slots = slots;
    D->size = 0;

    D->hash_table = (DictList *) calloc(slots, sizeof(DictList));

    for (int i = 0; i < D->slots; i++) {
        D->hash_table[i].head = NULL;
    }
    return D;
}

int insert(dictionary *D, char *key) {
    if (find_dict(D, key) != NULL) {
        warnx("Element with key '%s' already exists in dictionary", key);
        return -1;
    }

    DictNode *node = (DictNode *) malloc(sizeof(DictNode));
    memset(node->key, '\0', 32);
    memcpy(node->key, key, 32);
    node->queue = create_queue(128);

    node->prev = node->next = NULL;

    int index = hash(key, D->slots);

    if (insert_dict_list(&(D->hash_table[index]), node)) {
        D->size++;
        return 1;
    }
    return -1;
}

int del(dictionary *D, char *key) {
    DictNode *node = NULL;
    if ((node = find_dict(D, key)) == NULL) {
        warnx("Element with key '%s' is not in dictionary", key);
        return -1;
    }
    int index = hash(key, D->slots);
    if (delete_dict_list(&(D->hash_table[index]), node)) {
        D->size--;
        return 1;
    }
    return -1;
}

DictNode *find_dict(dictionary *D, char *key) {
    int index = hash(key, D->slots);
    return find_dict_list(&(D->hash_table[index]), key);
}

int hash(char key[32], int slots) {
    int num = 0;
    for (int i = 0; i < 32; i++) {
        num += (int) key[i] * 7;
    }
    return num % slots;
}

void print_dict(dictionary *D) {
    for (int i = 0; i < D->slots; i++) {
        print_dict_list(&(D->hash_table[i]));
    }
}

/*
testdict:		dictionary.o list.o
				$(CC) -o dict dictionary.o list.o

int main() {
    dictionary *D = create(5);
    insert(D, "tests/foo.txt");
    find_dict(D, "tests/foo.wtxt");
    char buf[32];

    for (int i = 0; i < 5; i++) {
        snprintf(buf, 32, "test%d.txt", i);
        insert(D, buf);
    }

    print_dict(D);

    printf("\nDeleting\n\n");

    for (int i = 0; i < 5; i++) {
        snprintf(buf, 32, "test%d.txt", i);
        del(D, buf);
    }



    print_dict(D);

    return 1;
}
*/

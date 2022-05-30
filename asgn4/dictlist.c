#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/file.h>
#include <unistd.h>
#include <err.h>

#include "dictlist.h"

//DictNode

//Dictionary Nodes to resolve hash collisions

DictNode *find_dict_list(DictList *list, char *key) {
    DictNode *head = list->head;
    DictNode *ptr = head;
    while (ptr != NULL) {
        if (strncmp(ptr->key, key, 32) == 0) {
            return ptr;
        }
        else {
            ptr = ptr->next;
        }
    }
    return NULL;
}

int insert_dict_list(DictList *list, DictNode *node) {
    if (list->head == NULL) {
        list->head = node;
        list->head->next = NULL;
        list->head->prev = NULL;
    }
    else {
        node->next = list->head;
        list->head->prev = node;
        node->prev = NULL;
        list->head = node;
    }
    return 1;
}

int delete_dict_list(DictList *list, DictNode *node) {
    if (list->head == NULL || node == NULL) {
        warnx("Delete from empty list or NULL node");
        return -1;
    }
    if (list->head == node) {
        list->head = node->next;
    }
    else {
        node->prev->next = node->next;
        if (node->next != NULL) {
            node->next->prev = node->prev;
        }
    }
    node->next = NULL;
    node->prev = NULL;
    node = NULL;
    free(node);
    return 1;
}

void print_dict_list(DictList *list) {
    DictNode *ptr = list->head;
    if (ptr) {
        printf("List:");
        while (ptr != NULL) {
            printf(" %s ",ptr->key);
            ptr = ptr->next;
        }
        printf("\n");
    }
    
    return;
}

/*

testlist:		list.o
				$(CC) -o list list.o

int main() {
    DictNode **hashtable = (DictNode **)calloc(5, sizeof(DictNode));
    char buf[32];
    for (int i=0; i<5; i++) {
        DictNode *new = (DictNode *)malloc(sizeof(DictNode));
        snprintf(buf, 32, "Node %d", i);
        memcpy(new->key, buf, 32);
        insert_dict_list(&hashtable[0], new);
    }
    print_dict_list(&hashtable[0]);

    DictNode *del = find_dict_list(&hashtable[0], "Node 2");

    delete_dict_list(&hashtable[0], del);

    print_dict_list(&hashtable[0]);

    for (int i = 10; i>0;i--) {

        snprintf(buf, 32, "Node %d", i);
        DictNode *del = find_dict_list(&hashtable[0], buf);
        delete_dict_list(&hashtable[0], del);
        print_dict_list(&hashtable[0]);
    }
    return 1;
}
*/

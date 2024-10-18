/*
Author: Peter Gauld
Purpose: The linked list will store the color values
File: linkedList.c

Timeline:
20240808 - File created.
*/

#include "linkedList.h"
#include <stdio.h>

struct Marker* allocMarker(int startCol, int startRow, int endCol, int endRow, int colorKey) {
    struct Marker *m = malloc(sizeof(struct Marker));
    m->startCol = startCol;
    m->startRow = startRow;
    m->endCol = endCol;
    m->endRow = endRow;
    m->colorKey = colorKey;
    m->neuter = 0;
    return m;
}

struct Marker* cloneMarker(struct Marker *m) {
    return allocMarker(m->startCol, m->endCol, m->endCol, m->endRow, m->colorKey);
}

struct Node* allocNode(struct Marker *m) {
    struct Node *n = malloc(sizeof(struct Node));
    n->next = NULL;
    n->marker = m;
    return n;
}

int LL_empty(struct LinkedList *ll) {
    return ll->head == NULL;
}

void LL_append(struct LinkedList *ll, struct Marker *m) {
    struct Node *n = allocNode(m);
    if(LL_empty(ll))
        ll->head = n;
    else
        ll->tail->next = n;
    ll->tail = n;
}

void LL_insert(struct LinkedList *ll, struct Node *prev, struct Marker *m) {
    struct Node *n = allocNode(m);
    if(LL_empty(ll) || prev == ll->tail)
        LL_append(ll, m);
    else if(prev == NULL) {
        n->next = ll->head;
        ll->head = n;
    }
    else {
        n->next = prev->next;
        prev->next = n;
    }
}

struct Marker* LL_removeHead(struct LinkedList *ll) {
    struct Node *remove = ll->head;
    struct Marker *m = remove->marker;
    ll->head = ll->head->next;
    free(remove);
    return m;
}

struct Marker* LL_remove(struct LinkedList *ll, struct Node *prev, struct Node *remove) {
    struct Marker *m = remove->marker;
    if(prev == NULL)
        return LL_removeHead(ll);
    prev->next = remove->next;
    if(remove == ll->tail) {
        ll->tail = prev;
    }
    free(remove);
    return m;
}
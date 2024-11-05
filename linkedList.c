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
    m->low = startCol;
    m->high = endCol;
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

struct Marker* LL_remove(struct LinkedList *ll, struct Node *prev, struct Node **r) {
    struct Node *remove = *r;
    struct Marker *m = remove->marker;
    if(prev == NULL) {
        *r = ll->head->next; /* Since prev is NULL, r should become the new head after LL_removeHead */
        return LL_removeHead(ll);
    }
    prev->next = remove->next;
    *r = prev->next; /* The node r is pointing to should become the node after prev */
    if(remove == ll->tail)
        ll->tail = prev;
    free(remove);
    return m;
}

void printMarker(struct Marker *m) {
    printf("START COL: %i, START ROW: %i, END COL: %i, END ROW: %i, LOW: %i, HIGH: %i, KEY: %i\n", m->startCol, m->startRow, m->endCol, m->endRow, m->low, m->high, m->colorKey);
}

void LL_print(struct LinkedList *ll) {
    struct Node *n = ll->head;
    while(n != NULL) {
        printMarker(n->marker);
        n = n->next;
    }
}
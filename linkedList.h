/*
Author: Peter Gauld
Purpose: Header for linkedList.c
File: linkedList.h

Timeline:
20240810 - File created.
20240817 - Added marker.
*/

#include <stdlib.h>

struct Marker {
    int startCol;
    int startRow;
    int endCol;
    int endRow;
    int low; /* Lower limit for the startCol. */
    int high; /* Upper limit for the endCol. */
    int colorKey; /* A number used as a key to identify the color of the node. */
    int neuter; /* Determines if a marker should be used to create more commands. */
};

struct Node { /* The node helps keep track of important information to write commands. */
    struct Node *next;
    struct Marker *marker;
};

struct LinkedList {
    struct Node *head;
    struct Node *tail;
};

struct Marker* allocMarker(int startCol, int startRow, int endCol, int endRow, int colorKey);
struct Marker* cloneMarker(struct Marker*);
int LL_empty(struct LinkedList *ll);
void LL_append(struct LinkedList *ll, struct Marker *m);
void LL_insert(struct LinkedList *ll, struct Node *prev, struct Marker *m);
struct Marker* LL_removeHead(struct LinkedList *ll);
struct Marker* LL_remove(struct LinkedList *ll, struct Node *prev, struct Node **remove);
void LL_print(struct LinkedList *ll);
void LL_printMarker(struct Marker *m);
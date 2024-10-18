/*
Name: Peter Gauld
Purpose: Implement a quad data structure.
File: quad.h

Timeline:
20240921 - File  created
*/

#include <stdlib.h>

struct Quad {
    struct Quad *children[4];
    int color;
    int size;
    int leaf;
    int row;
    int col;
};

struct Quad buildQuad(int **grid, int colors, int size);
struct Quad buildQuadOG(int **grid, int size);
void destroyQuad(struct Quad *q);
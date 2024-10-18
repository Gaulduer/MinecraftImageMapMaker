/*
Name: Peter Gauld
Purpose: Implement a quad data structure.
File: quad.c

Timeline:
20240921 - File  created
20240923 - Quad now properly zero's out 'counts' array before using it.
*/

#include "quad.h"
#include "stdio.h"

int buildQuadHelperOG(struct Quad *q, int **grid, int row, int col, int size) {
    int i, childSize = size/2, colors[4], counts[4], dominantCount = 0;
    q->row = row;
    q->col = col;
    q->size = size;

    if(size == 1) { /* When you can't go deeper, return. */
        q->color = grid[row][col];
        for(i = 0 ; i < 4 ; i++)
            q->children[i] == NULL;
        q->leaf = 1;
        return q->color;
    }
    q->leaf = 0;

    for(i = 0 ; i < 4 ; i++)
        q->children[i] = malloc(sizeof(struct Quad));

    colors[0] = buildQuadHelperOG(q->children[0], grid, row, col, childSize);
    colors[1] = buildQuadHelperOG(q->children[1], grid, row, col + childSize, childSize);
    colors[2] = buildQuadHelperOG(q->children[2], grid, row + childSize, col, childSize);
    colors[3] = buildQuadHelperOG(q->children[3], grid, row + childSize, col + childSize, childSize);

    for(i = 0 ; i < 4 ; i++) {
        int j;
        counts[i] = 0; /* Set to zero each time to avoid recounts and also just zero it out. */
        for(j = 0 ; j < 4 ; j++)
            if(colors[i] == colors[j])
                counts[i]++;
    }

    for(i = 0 ; i < 4 ; i++) {
        if(counts[i] > dominantCount) {
            q->color = colors[i]; 
            dominantCount = counts[i];
        }
    }

    return q->color;
}

void buildQuadHelper(struct Quad *q, int **grid, int *parentCounts, int colors, int row, int col, int size) {
    int i, childSize = size/2, *counts;
    q->row = row;
    q->col = col;
    q->size = size;

    if(size == 1) { /* When you can't go deeper, return. */
        q->color = grid[row][col];
        q->leaf = 1;
        parentCounts[q->color]++;
        printf("Root Quad Color: %i\n", q->color);
        return;
    }
    q->leaf = 0;

    counts = malloc(colors * sizeof(int));

    for(i = 0 ; i < colors ; i++)
        counts[i] = 0;

    for(i = 0 ; i < 4 ; i++)
        q->children[i] = malloc(sizeof(struct Quad));

    buildQuadHelper(q->children[0], grid, counts, colors, row, col, childSize);
    buildQuadHelper(q->children[1], grid, counts, colors, row, col + childSize, childSize);
    buildQuadHelper(q->children[2], grid, counts, colors, row + childSize, col, childSize);
    buildQuadHelper(q->children[3], grid, counts, colors, row + childSize, col + childSize, childSize);

    q->color = 0;
    for(i = 1 ; i < colors ; i++) /* The color with the greatest count will decide the color of this quad. */
        if(counts[i] > counts[q->color])
            q->color = i;

    printf("COLORS: [ ");
    for(i = 0 ; i < 3 ; i++) 
        printf("%i, ", counts[i]);
    printf("%i ]\n", counts[3]);
    for(i = 0 ; i < colors ; i++) /* Consolidating colors */
        parentCounts[i] += counts[i];

    printf("Quad Color: %i\n", q->color);
    free(counts);
}

struct Quad buildQuadOG(int **grid, int size) {
    struct Quad quad;
    buildQuadHelperOG(&quad, grid, 0, 0, size);
    return quad;
}

struct Quad buildQuad(int **grid, int colors, int size) {
    int * counts = malloc(colors * sizeof(int));
    struct Quad quad;
    buildQuadHelper(&quad, grid, counts, colors, 0, 0, size);
    free(counts);
    return quad;
}

void destroyQuadHelper(struct Quad *q) {
    int i;

    if(q->leaf)
        return;

    for(i = 0 ; i < 4 ; i++) {
        destroyQuadHelper(q->children[i]);
        free(q->children[i]);
    }
}

void destroyQuad(struct Quad *q) {
    destroyQuadHelper(q);
}
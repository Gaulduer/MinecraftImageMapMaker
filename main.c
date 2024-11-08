/*
Author: Peter Gauld
Project: Minecraft Map Image Maker.
File: main.c

Timeline:
20240726 - Project created. Continuation of a previous project.
20240803 - Original bitmap successfully displayed. (May not work for all bitmaps just yet.)
20240804 - Preview image generated. RGB values succesfully translated to blocks.
20240810 - Changed the emergency stop on command input. Now pauses input and allows it to resume without need to restart program.
20240816 - Much deliberation made over command generation methods.
20240904 - Begin button is now functional, triggering inputCommands instead of having it run automatically.
20240922 - Added a quad function to help with commands.
20241014 - Added a way to optimize quad function commands
20241017 - Horizontal priority optimization added.
20241023 - Changed logic back to logic that was working previously.
20241023 - Improved command input method.
20241108 - Finished implementing a testing system. This will help provide information on any errors created by command optimization.
Highlights any errors that will appear in the final resulting image. I intend to make a separate highlight that shows the area of the color that is correct.
*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "bmp.h"
#include "linkedList.h"
#include "quad.h"

#define LINE_LIMIT 128

uint32_t* allocScaledPixels(uint32_t *pixels, int width, int height, int scale) {
    uint32_t *scaled = malloc(width * scale * height * scale * sizeof(uint32_t));
    int i, j, k, ip = 0, is = 0;
    while(ip < width * height) {
        for(i = 0 ; i < scale ; i++) {
            for(j = ip ; j < ip + width ; j++)
                for(k = 0 ; k < scale ; k++)
                    scaled[is++] = pixels[j];
        }
        ip = j;
    }

    return scaled;
}

uint32_t* allocSolidColor(int width, int height, uint32_t color) {
    uint32_t *pixels = malloc(width * height * sizeof(uint32_t));
    int i;

    for(i = 0 ; i < width * height ; i++)
        pixels[i] = color;
    return pixels;
};

void fillRectangle(HDC hdc, uint32_t *pixels, int left, int top, int right, int bottom, int scale) {
    int width = right - left, height = bottom - top;
    uint32_t *scaledPixels = allocScaledPixels(pixels, width, height, scale);
    BITMAPINFO bmi;
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = width * scale;
    bmi.bmiHeader.biHeight = height * scale;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    StretchDIBits(hdc, left * scale, top * scale, width * scale, height * scale, 0, 0, width * scale, height * scale, scaledPixels, &bmi, DIB_RGB_COLORS, SRCCOPY);

    free(scaledPixels);
}

void outlineRectangle(HDC hdc, int left, int top, int right, int bottom, int scale) {
    uint32_t *line = allocSolidColor(right - left, 1, 0x00FF00FF);
    fillRectangle(hdc, line, left, top, right, top + 1, scale);
    fillRectangle(hdc, line, left, bottom - 1, right, bottom, scale);
    free(line);
    line = allocSolidColor(1, bottom - top, 0x00FF00FF);
    fillRectangle(hdc, line, left, top, left + 1, top, scale);
    fillRectangle(hdc, line, right - 1, bottom, right, bottom, scale);
    free(line);
}

void inputCommand(HWND selectedWindow, char *command) {
	const size_t len = strlen(command) + 1;
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
    memcpy(GlobalLock(hMem), command, strlen(command) + 1);
	GlobalUnlock(hMem);
	OpenClipboard(0);
	EmptyClipboard();
	SetClipboardData(CF_TEXT, hMem);
	CloseClipboard();

    if(GetAsyncKeyState(VK_RIGHT))
        while(!GetAsyncKeyState(VK_RIGHT));
    SendMessageW(selectedWindow, WM_KEYDOWN, 'T', 0);
    Sleep(60);
    SendMessageW(selectedWindow, WM_KEYDOWN, VK_CONTROL, 0);
    SendMessageW(selectedWindow, WM_KEYDOWN, 'V', 0);
    SendMessageW(selectedWindow, WM_KEYDOWN, VK_RETURN, 0);
}

void simulateCommand(HDC hdc, struct Marker *m, uint32_t color, int xOffset, int yOffset) {
    uint32_t *pixels = allocSolidColor(m->endCol - m->startCol + 1, m->endRow - m->startRow + 1, color);
    fillRectangle(hdc, pixels, m->startCol + xOffset, m->startRow + yOffset, m->endCol + 1 + xOffset, m->endRow + 1 + yOffset, 2);
    free(pixels);
}

void inputCommands(HDC hdc) {
    FILE *commands = fopen("commands.txt", "r"), *pixelColors = fopen("pixelColors.txt", "r");
    INPUT ip;
    HWND selectedWindow;
    WCHAR name[128];
    char command[512];
    struct Marker m;
    uint32_t color;

    GetAsyncKeyState(VK_CONTROL); /* In case control was pressed before. */
    while(!GetAsyncKeyState(VK_CONTROL));
    selectedWindow = GetForegroundWindow();
    GetWindowTextW(selectedWindow, name, 128);
    printf("Selected Window - %ls\n", name);

    Sleep(1000);

	while(fgets(command, 512, commands)) {
        fscanf(pixelColors, "%u", &color);
        sscanf(command, "/fill ~%i ~-1 ~%i ~%i ~-1 ~%i", &m.startCol, &m.startRow, &m.endCol, &m.endRow);
        inputCommand(selectedWindow, command);
        simulateCommand(hdc, &m, color, 128, 0);
    }

    fclose(pixelColors);
    fclose(commands);
}


void imprintGrid(struct LinkedList *queue, int **grid) {
    int i, j;
    struct Node *n = queue->head;
    while(n != NULL) {
        for(i = n->marker->startRow ; i <= n->marker->endRow ; i++) {
            for(j = n->marker->startCol ; j <= n->marker->endCol ; j++) {
                grid[i][j] = n->marker->colorKey;
            }
        }
        n = n->next;
    }
}

int **allocGrid() {
    int i, j, **grid = malloc(128 * sizeof(int*));
    for(i = 0 ; i < 128 ; i++) {
        grid[i] = malloc(128 * sizeof(int));
        for(j = 0 ; j < 128 ; j++)
            grid[i][j] = -1; /* Setting to negative 1 since that is an impossible index. */
    }
    return grid;
}

void freeGrid(int **grid) {
    int i;
    for(i = 0 ; i < 128 ; i++)
        free(grid[i]);
    free(grid); 
}

int gridsMatch(int **grid1, int **grid2) {
    int i, j;
    for(i = 0 ; i < 128 ; i++)
        for(j = 0 ; j < 128 ; j++)  
            if(grid1[i][j] != grid2[i][j]) { /* Grids do not match */
                printf("Mismatch at (%i, %i)\n", j, i);
                return 0;
            }
    return 1;
}

int getDiff(struct RGBColor c1, struct RGBColor c2) {
    return (c1.r > c2.r ? (c1.r - c2.r):(c2.r - c1.r)) + (c1.g > c2.g ? (c1.g - c2.g):(c2.g - c1.g)) + (c1.b > c2.b ? (c1.b - c2.b):(c2.b - c1.b));
}

int selectColorIndex(struct RGBColor compare, FILE *colorKey) {
    char block[128], blockBuffer[128];
    int line = 0, index, diff, diffBuffer, r, g, b;
    struct RGBColor rgb, rgbBuffer;

    rewind(colorKey);
    fscanf(colorKey, "%i,%i,%i,%s", &rgb.r, &rgb.g, &rgb.b, block);
    diff = getDiff(compare, rgb);
    while(fscanf(colorKey, "%i,%i,%i,%s", &r, &g, &b, blockBuffer) != EOF) { /* I use buffer values for the buffer itself since somehow rgb kept getting zeroed out when rgbBuffer was used directly. */
        line++;
        rgbBuffer.r = r;
        rgbBuffer.g = g;
        rgbBuffer.b = b;
        diffBuffer = getDiff(compare, rgbBuffer);
        if(diffBuffer < diff) {
            rgb = rgbBuffer;
            strcpy(block, blockBuffer);
            diff = diffBuffer;
            index = line;
        }
    }

    return index;
}

COLORREF rgbFromIndex(int index, FILE *colorKey) {
    int i, r, g, b;
    char buffer[128];

    if(index < 0)
        return RGB(255, 0, 255);

    rewind(colorKey);
    for(i = 0 ; i <= index ; i++)
        fscanf(colorKey, "%i,%i,%i,%s", &r, &g, &b, buffer);
    
    return RGB(r, g, b);
}

void writeCommand(FILE *commands, struct Marker *marker, char **colors) {
    fprintf(commands, "/fill ~%i ~-1 ~%i ~%i ~-1 ~%i minecraft:%s\n", marker->startCol, marker->startRow, marker->endCol, marker->endRow, colors[marker->colorKey]);
}

struct Marker* allocQuadMarker(struct Quad *q) {
    return allocMarker(q->col, q->row, q->col + q->size - 1, q->row + q->size - 1, q->color);
}

int quad(struct Quad *q, struct LinkedList *parentQueue, int *layers, int limit, int layer) {
    struct LinkedList queue = {NULL, NULL};
    int i, priority, priorities[4];
    if(q->size <= limit || q->leaf) {
        LL_append(parentQueue, allocQuadMarker(q));
        return layers[q->color];
    }

    if(layers[q->color] > layer) /* This if statement ensures a layer for a color wont be reset unless it was still at __INT_MAX__ */
        layers[q->color] = layer;
    priority = layers[q->color];

    for(i = 0 ; i < 4 ; i++)
        priorities[i] = quad(q->children[i], &queue, layers, limit, layer + 1);

    for(i = 0 ; i < 4 ; i++)
        if(priorities[i] < priority)
            priority = priorities[i]; 

    /* Note do not add a condition such as layer == layers[q->color]. Doing this will let us avoid picking out colors, but we cannot put colors 'back in'. If a higher layer command falls through, necessary commands will be missing. */
    if(priority >= layers[q->color]) { /* If the priority is not more signifcant than the layer, we can cover this area with a single command for the color. */
        LL_append(parentQueue, allocQuadMarker(q));
        while(!LL_empty(&queue)) { /* Picking out extra commands for this layer's color. */
            if(queue.head->marker->colorKey == q->color) /* Toss out all nodes that match this layer's color. */
                free(LL_removeHead(&queue));
            else
                LL_append(parentQueue, LL_removeHead(&queue));
        }
    }
    else 
        while(!LL_empty(&queue))
            LL_append(parentQueue, LL_removeHead(&queue));

    if(layer == layers[q->color])
        layers[q->color] = __INT_MAX__;

    return priority;
}

void prioritizeMarkers(struct LinkedList *priorityQueue, int *counters, int n) {
    int i;
    for(i = 0 ; i < n ; i++) {
        if(counters[i] == 0)
            continue;
        if(LL_empty(priorityQueue) || counters[priorityQueue->tail->marker->colorKey] >= counters[i])
            LL_append(priorityQueue, allocMarker(0, 0, 0, 0, i));
        else {
            struct Node *prev = NULL, *n = priorityQueue->head;
            while(counters[n->marker->colorKey] >= counters[i]) {
                prev = n;
                n = n->next;
            }
            LL_insert(priorityQueue, prev, allocMarker(0, 0, 0, 0, i));
        }
    }
}

void mergeRow(struct LinkedList *row, int c) { /* Merges markers of color c in a single row together horizontally. */
    int low = 0;
    struct Node *prev = NULL, *n = row->head;
    while(n != NULL) {
        struct Marker *m1 = n->marker;
        prev = n;
        n = n->next;
        if(m1->colorKey != c)
            continue;
        m1->low = low;
        m1->neuter = 1; /* Ensures other markers will be stopped when they encounter this one. */
        while(n != NULL) {
            struct Marker *m2 = n->marker;
            if(m1->high + 1 == m2->startCol && !m2->neuter) {
                m1->high = m2->endCol;
                if(m2->colorKey == c) {
                    m1->endCol = m2->endCol;
                    free(LL_remove(row, prev, &n));
                }
                else {
                    prev = n;
                    n = n->next;
                }
            }
            else
                break;
        }
    }
}

void mergeTwoRows(struct LinkedList *row1, struct LinkedList *row2, struct LinkedList *q, int c) { /* 'Zippering' markers of the same color together between two rows. A vertical merge. */
    struct Node *prev1 = NULL, *n1 = row1->head, *prev2 = NULL, *n2 = row2->head, *prevContact = NULL, *contact = row2->head;
    
    while(n1 != NULL && n2 != NULL) {
        prev2 = prevContact;
        n2 = contact;

        if(n1->marker->colorKey != c) {
            prev1 = n1;
            n1 = n1->next;
            continue;
        }

        while(n2 != NULL && n2->marker->endCol < n1->marker->low) {
            prev2 = n2;
            n2 = n2->next;
        }

        if(n2 == NULL)
            break;

        prevContact = prev2;
        contact = n2;

        while(n2->marker->endCol < n1->marker->startCol && n2->next != NULL) {
            if(n2->marker->endCol + 1 == n2->next->marker->startCol) {
                n1->marker->low = n1->marker->low > n2->next->marker->startCol ? n1->marker->low:n2->next->marker->startCol;
                prevContact = prev2;
                contact = n2;
            }
            prev2 = n2;
            n2 = n2->next;
        }

        if(contact->marker->startCol > n1->marker->startCol) {
            prev1 = n1;
            n1 = n1->next;
            continue;
        }

        while(n2->marker->endCol < n1->marker->endCol && n2->next != NULL && n2->marker->endCol + 1 == n2->next->marker->startCol) {
            prev2 = n2;
            n2 = n2->next;
        }

        if(n2->marker->endCol < n1->marker->endCol) {
            prev1 = n1;
            n1 = n1->next;
            continue;
        }

        while(n2->marker->endCol < n1->marker->high && n2->next != NULL && n2->marker->endCol + 1 == n2->next->marker->startCol) {
            prev2 = n2;
            n2 = n2->next;
        }

        n1->marker->high = n1->marker->high < n2->marker->endCol ? n1->marker->high:n2->marker->endCol;
        n1->marker->endRow = n2->marker->endRow;

        prev2 = prevContact;
        n2 = contact;

        while(n2 != NULL && n2->marker->endCol <= n1->marker->high) {
            if(n2->marker->colorKey == c) {
                n1->marker->startCol = n1->marker->startCol < n2->marker->startCol ? n1->marker->startCol:n2->marker->startCol;
                n1->marker->endCol = n1->marker->endCol > n2->marker->endCol ? n1->marker->endCol:n2->marker->endCol;
                free(LL_remove(row2, prev2, &n2));
            }
            else {
                prev2 = n2;
                n2 = n2->next;
            }
        }

        LL_insert(row2, prevContact, LL_remove(row1, prev1, &n1));
    }
}

void mergeCommands(struct LinkedList *lines, struct LinkedList *q, int size, int colors) {
    int i, inc = 1, *counters = malloc(colors * sizeof(int));
    struct LinkedList priorityQueue = {NULL, NULL}, commandQueue = {NULL, NULL};
    struct Node *priority;

    for(i = 0 ; i < colors ; i++)
        counters[i] = 0;

    while(!LL_empty(q)) {
        struct Marker *m = q->head->marker;
        counters[m->colorKey]++;
        LL_append(&lines[m->startRow], LL_removeHead(q));
    }     

    prioritizeMarkers(&priorityQueue, counters, colors);

    priority = priorityQueue.head;
    while(priority != NULL) {
        int c = priority->marker->colorKey;
        priority = priority->next;
        for(i = 0 ; i < LINE_LIMIT ; i++)
            mergeRow(&lines[i], c);
    }

    priority = priorityQueue.head;
    while(priority != NULL) {
        struct Node *prev, *n;
        int c = priority->marker->colorKey, r1 = 0, r2 = 0;
        priority = priority->next;
        while(r2 < LINE_LIMIT) {
            r1 = r2;
            r2 = r1 + size;

            if(LL_empty(&lines[r1]))
                continue;

            if(r2 < LINE_LIMIT)
                mergeTwoRows(&lines[r1], &lines[r2], q, c);

            prev = NULL, n = lines[r1].head;
            while(n != NULL) {
                if(n->marker->colorKey == c)
                    LL_append(q, LL_remove(&lines[r1], prev, &n));
                else {
                    prev = n;
                    n = n->next;
                }
            }
        }
    }

    while(!LL_empty(&priorityQueue))
        free(LL_removeHead(&priorityQueue));

    free(counters);
}

void optimizeCommands(struct LinkedList *queue, int colors) {
    int i, size = 128, sizes[8];
    struct LinkedList priorityQueue = {NULL, NULL}, cats[8], lines[128]; /* These linked lists seperate commands based on their size category. */
    for(i = 0 ; i < 8 ; i++) {
        sizes[i] = size;
        size /= 2;
        cats[i].head = NULL;
        cats[i].tail = NULL;
    }
    for(i = 0 ; i < 128 ; i++) {
        lines[i].head = NULL;
        lines[i].tail = NULL;
    }

    while(!LL_empty(queue)) {
        struct Marker *m = queue->head->marker;
        int width = (m->endCol - m->startCol + 1);
        for(i = 0 ; i < 8 ; i++) {
            if(sizes[i] == width)
                LL_append(&cats[i], LL_removeHead(queue));
        }
    } 

    for(i = 0 ; i < 8 ; i++)
        mergeCommands(lines, &cats[i], sizes[i], colors);

    for(i = 0 ; i < 8 ; i++) {
        while(!LL_empty(&cats[i]))
            LL_append(queue, LL_removeHead(&cats[i]));
    }
}

void testCommands(HDC hdc, uint32_t *pixelKey, int **original, int **optimized, struct LinkedList *q) {
    int i, j, count = 1;
    struct Node *n = q->head;
    while(n != NULL) {
        int wrong = 0;
        struct Marker *m = n->marker;
        uint32_t *pixels = allocSolidColor(m->endCol - m->startCol + 1, m->endRow - m->startRow + 1, pixelKey[m->colorKey]);
        for(i = m->startRow ; i <= m->endRow ; i++)
            for(j = m->startCol ; j <= m->endRow ; j++) {
                if(original[i][j] != optimized[i][j] && optimized[i][j] == m->colorKey) {
                    wrong = 1;
                    printf("MISCOLOR AT [%i, %i]\n", j, i);
                }
            }
        
        if(wrong) {
            uint32_t *magenta = allocSolidColor(m->endCol - m->startCol + 1, m->endRow - m->startRow + 1, 0x00FF00FF);
            
            printf("INCORRECT LINE: %i", count);
            fillRectangle(hdc, magenta, m->startCol + 128, m->startRow, m->endCol + 1 + 128, m->endRow + 1, 2); /* Fill the rectangle back in to hide outline. */    
            free(magenta);
            getc(stdin); /* Pause for viewing */
        }
        fillRectangle(hdc, pixels, m->startCol + 128, m->startRow, m->endCol + 1 + 128, m->endRow + 1, 2); /* Fill the rectangle back in to hide outline. */
        free(pixels);

        n = n->next;
        count++; /* Keeps track of which marker we are on. */
    }
}

void generateCommands(struct Quad q, char **colors, uint32_t *pixelKey, HDC hdc) {
    FILE *commands = fopen("commands.txt", "w"), *pixelColors = fopen("pixelColors.txt", "w");
    struct LinkedList commandQueue = {NULL, NULL};
    int i, n = 0, *layers, **originalGrid = allocGrid(), **optimizedGrid = allocGrid();

    while(colors[i++] != NULL);
    n = i - 1;

    layers = malloc(n * sizeof(int));

    for(i = 0 ; i < n ; i++)
        layers[i] = __INT_MAX__;

    quad(&q, &commandQueue, layers, 2, 0);
    imprintGrid(&commandQueue, originalGrid);
    optimizeCommands(&commandQueue, n);
    imprintGrid(&commandQueue, optimizedGrid);
    testCommands(hdc, pixelKey, originalGrid, optimizedGrid, &commandQueue);

    while(!LL_empty(&commandQueue)) {
        fprintf(pixelColors, "%u\n", pixelKey[commandQueue.head->marker->colorKey]);
        writeCommand(commands, commandQueue.head->marker, colors);
        free(LL_removeHead(&commandQueue));
    }
    fclose(commands);
    fclose(pixelColors);
    freeGrid(originalGrid);
    freeGrid(optimizedGrid);
    free(layers);
}

int amountOfColors(FILE *colorKey) {
    int i = 0, i1, i2, i3, n = 0;
    char s[128];

    rewind(colorKey);
    while(fscanf(colorKey, "%i,%i,%i,%s", &i1, &i2, &i3, s) != EOF) /* Figuring out how many colors should be in the key. */
        n++;
    return n;
}

char** getColorNames(FILE *colorKey, int n) {
    int i = 0, i1, i2, i3;
    char **colors;
    
    colors = malloc((n + 1) * sizeof(char*));
    rewind(colorKey);
    for(i = 0 ; i < n ; i++) {
        colors[i] = malloc(128 * sizeof(char));
        fscanf(colorKey, "%i,%i,%i,%s", &i1, &i2, &i3, colors[i]);
    }
    colors[n] = NULL;
    return colors;
}

uint32_t* getPixelKey(FILE *colorKey, int n) {
    int i = 0;
    char buffer[64];
    struct RGBColor rgb;
    uint32_t *pixels = malloc(n * sizeof(uint32_t));
    
    rewind(colorKey);
    for(i = 0 ; i < n ; i++) {
        fscanf(colorKey, "%i,%i,%i,%s", &rgb.r, &rgb.g, &rgb.b, buffer);
        pixels[i] = rgbToPixel(rgb);
    }

    return pixels;
}

void readImage(HDC hdc, int scale, char *image, char *key) {
    FILE *fr = fopen(image, "rb");
    FILE *colorKey = fopen(key, "r");
    FILE *colorIndices = fopen("colorIndices.txt", "w");
    int **grid = allocGrid();
    int transparency = 0, i, j, p = 0, n = amountOfColors(colorKey);
    struct BMPHeader h = readBMPHeader(fr);
    struct RGBColor rgb, minecraftRGB, *minecraftRGBs;
    struct Quad q;
    char **colors = getColorNames(colorKey, n);
    uint32_t *pixels = malloc(128 * 128 * sizeof(uint32_t)), *pixelKey = getPixelKey(colorKey, n), pixel = 0;

    if(h.bitsPerPixel > 24)
        transparency = 1;

    for(i = 0 ; i < 128 * 128 ; i++) 
        pixels[i] = readPixel(fr, transparency);

    fillRectangle(hdc, pixels, 0, 0, 128, 128, 2);

    for(i = 0 ; i < 128 ; i++) {
        for(j = 0 ; j < 128 ; j++)
            grid[i][j] = selectColorIndex(pixelToRGB(pixels[(127 - i) * 128 + j]), colorKey);
    }

    q = buildQuadOG(grid, 128);
    generateCommands(q, colors, pixelKey, hdc);
    destroyQuad(&q);
    freeGrid(grid);
    for(i = 0 ; colors[i] != NULL ; i++)
        free(colors[i]);
    free(colors);
    free(pixels);
    free(pixelKey);
    fclose(fr);
    fclose(colorKey);
    fclose(colorIndices);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch(msg) {
        case WM_CLOSE:
            PostMessageW(hwnd, WM_QUIT, 0, 0);
        case WM_COMMAND: {
            switch(wp) {
                case 0: {
                    printf("Inputing commands...\n");
                    inputCommands(GetDC(hwnd));
                } break;
                default:
                    printf("No command associated with WP %i", wp);
            }
        }
        default: 
            return DefWindowProcW(hwnd, msg, wp, lp);
    }
    return 0;
}

void mergeTest(HDC hdc, char *key) {
    FILE *colorKey = fopen(key, "r");
    int i, j, inc = 1, n = 59, *counters = malloc(n * sizeof(int)), **grid = allocGrid();
    struct LinkedList priorityQueue = {NULL, NULL}, commandQueue = {NULL, NULL}, *q = &commandQueue, lines[LINE_LIMIT];
    struct Node *priority;

    for(i = 0 ; i < LINE_LIMIT ; i++) {
        lines[i].head = NULL;
        lines[i].tail = NULL;
    }

    for(i = 0 ; i < n ; i++)
        counters[i] = 0;

    for(i = 0 ; i < 128 ; i++)
        for(j = 0 ; j < 128 ; j++)
            LL_append(q, allocMarker(j, i, j, i, 8));

    LL_print(q);

    imprintGrid(q, grid);

    while(!LL_empty(q)) {
        struct Marker *m = q->head->marker;
        counters[m->colorKey]++;
        LL_append(&lines[m->startRow], LL_removeHead(q));
    }     

    prioritizeMarkers(&priorityQueue, counters, n);

    priority = priorityQueue.head;
    while(priority != NULL) {
        int c = priority->marker->colorKey;
        priority = priority->next;
        for(i = 0 ; i < LINE_LIMIT ; i++)
            mergeRow(&lines[i], c);
    }

    printf("HORIZONTAL:\n");
    for(i = 0 ; i < LINE_LIMIT ; i++) {
        printf("%i\n", i);
        LL_print(&lines[i]);
    }

    priority = priorityQueue.head;
    if(!LL_empty(&lines[0])) /* Determining how much to increment by between lines. */
        inc = lines[0].head->marker->endRow + 1;
    while(priority != NULL) {
        struct Node *prev, *n;
        int c = priority->marker->colorKey, r1 = 0, r2 = 0;
        priority = priority->next;
        while(r2 < LINE_LIMIT) {
            r1 = r2;
            r2 = r1 + inc;

            if(LL_empty(&lines[r1]))
                continue;

            if(r2 < LINE_LIMIT)
                mergeTwoRows(&lines[r1], &lines[r2], q, c);

            prev = NULL, n = lines[r1].head;
            while(n != NULL) {
                if(n->marker->colorKey == c)
                    LL_append(q, LL_remove(&lines[r1], prev, &n));
                else {
                    prev = n;
                    n = n->next;
                }
            }
        }
        printf("ENDING ROWS: %i %i\n", r1, r2);
        printf("AFTER %i\n", c);
        for(i = 0 ; i < LINE_LIMIT ; i++) {
            printf("%i\n", i);
            LL_print(&lines[i]);
        }
    }

    printf("RESULTS:\n");
    LL_print(q);

    //testCommands(q, hdc, grid);

    while(!LL_empty(q))
        free(LL_removeHead(q));

    freeGrid(grid);
    free(counters);
}

void stringTest() {
    char *command = "/fill ~0 ~-1 ~0 ~125 ~-1 ~127 minecraft:gray_terracotta";
    struct Marker m;

    sscanf(command, "/fill ~%i ~-1 ~%i ~%i ~-1 ~%i", &m.startCol, &m.startRow, &m.endCol, &m.endRow);

    printf("%i %i %i %i\n", m.startCol, m.startRow, m.endCol, m.endRow);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmd, int cmdShow) {
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"main";
    wc.lpszMenuName = L"Minecraft Image Map Maker";
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(wc.lpszClassName, wc.lpszMenuName, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, 528, 394, NULL, NULL, NULL, NULL);
    CreateWindowW(L"Button", L"Begin", WS_VISIBLE | WS_CHILD , 0, 256, 528, 100, hwnd, 0, NULL, NULL);

    readImage(GetDC(hwnd), 2, "gurt.bmp", "colorKeys\\all.csv");
    //mergeTest(GetDC(hwnd), "colorKeys\\all.csv");
    //rgbTest(GetDC(hwnd), "colorKeys\\all.csv");
    //stringTest();

    MSG msg = {};
    msg.hwnd = hwnd;
    while(GetMessage(&msg, 0, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
} 
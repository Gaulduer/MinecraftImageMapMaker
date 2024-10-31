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
*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "bmp.h"
#include "linkedList.h"
#include "quad.h"

void inputCommands() {
    FILE *commands = fopen("commands.txt", "r");
    INPUT ip;
    HWND selectedWindow;
    WCHAR name[128];
    char command[512];

    GetAsyncKeyState(VK_CONTROL); /* In case control was pressed before. */
    while(!GetAsyncKeyState(VK_CONTROL));
    selectedWindow = GetForegroundWindow();
    GetWindowTextW(selectedWindow, name, 128);
    printf("Selected Window - %ls\n", name);

    Sleep(1000);

	while(fgets(command, 512, commands)) {
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

    fclose(commands);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch(msg) {
        case WM_CLOSE:
            PostMessageW(hwnd, WM_QUIT, 0, 0);
        case WM_COMMAND: {
            switch(wp) {
                case 0: {
                    printf("Inputing commands...\n");
                    inputCommands();
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

int getDiff(struct RGBColor c1, struct RGBColor c2) {
    return (c1.r > c2.r ? (c1.r - c2.r):(c2.r - c1.r)) + (c1.g > c2.g ? (c1.g - c2.g):(c2.g - c1.g)) + (c1.b > c2.b ? (c1.b - c2.b):(c2.b - c1.b));
}

struct RGBColor selectRGB(struct RGBColor compare, int *index, FILE *colorKey) {
    char block[128], blockBuffer[128];
    int line = 0, diff, diffBuffer, r, g, b;
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
            *index = line;
        }
    }

    return rgb;
}

void fillRectangle(HDC hdc, COLORREF color, int left, int top, int right, int bottom) {
    for(int i = top ; i < bottom ; i++)
        for(int j = left ; j < right ; j++)
            SetPixel(hdc, j, i, color);
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

void mergeCommands(struct LinkedList *lines, struct LinkedList *queue, int size, int colors) {
    int i;

    /* Merging commands horizontally. */
    while(!LL_empty(queue)) {
        int placement = queue->head->marker->startRow;
        LL_append(&lines[placement], LL_removeHead(queue));
    }

    for(i = 0 ; i < 128 ; i++) {
        while(!LL_empty(&lines[i])) {
            struct Marker *m1 = lines[i].head->marker;
            LL_append(queue, LL_removeHead(&lines[i]));
            if(m1->endCol - m1->startCol + 1 != size)
                continue;
            while(!LL_empty(&lines[i])) {
                struct Marker *m2 = lines[i].head->marker;
                if(m1->endRow + 1 == m2->startRow && m1->colorKey == m2->colorKey) {
                    m1->endRow = m2->endRow;
                    if(m1->endCol == m2->endCol)
                        free(LL_removeHead(&lines[i]));
                    else
                        LL_append(queue, LL_removeHead(&lines[i]));
                }
                else
                    break;
            }
        }
    }

    /* Merging commands vertically. */
    while(!LL_empty(queue)) {
        int placement = queue->head->marker->startCol;
        LL_append(&lines[placement], LL_removeHead(queue));
    }

    for(i = 0 ; i < 128 ; i++) {
        while(!LL_empty(&lines[i])) {
            struct Marker *m1 = lines[i].head->marker;
            LL_append(queue, LL_removeHead(&lines[i]));
            while(!LL_empty(&lines[i])) {
                struct Marker *m2 = lines[i].head->marker;
                if(m1->endCol + 1 == m2->startCol && m1->colorKey == m2->colorKey) {
                    m1->endCol = m2->endCol;
                    free(LL_removeHead(&lines[i]));
                }
                else
                    break;
            }
        }
    }
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

void generateCommands(struct Quad q, char **colors) {
    FILE *commands = fopen("commands.txt", "w");
    struct LinkedList commandQueue = {NULL, NULL};
    int i, n = 0, *layers;

    while(colors[i++] != NULL);
    n = i - 1;

    layers = malloc(n * sizeof(int));

    for(i = 0 ; i < n ; i++)
        layers[i] = __INT_MAX__;

    quad(&q, &commandQueue, layers, 4, 0);
    optimizeCommands(&commandQueue, n);
    while(!LL_empty(&commandQueue)) {
        writeCommand(commands, commandQueue.head->marker, colors);
        free(LL_removeHead(&commandQueue));
    }
    fclose(commands);
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

void readImage(HDC hdc, int scale, char *image, char *key) {
    FILE *fr = fopen(image, "rb");
    FILE *colorKey = fopen(key, "r");
    FILE *colorIndices = fopen("colorIndices.txt", "w");
    struct BMPHeader h = readBMPHeader(fr);
    struct RGBColor rgb, minecraftRGB;
    struct Quad q;
    int **grid = malloc(128 * sizeof(int*));
    int transparency = 0, i, j, n = amountOfColors(colorKey);
    char **colors = getColorNames(colorKey, n);
    
    for(i = 0 ; i < 128 ; i++)
        grid[i] = malloc(128 * sizeof(int));

    if(h.bitsPerPixel > 24)
        transparency = 1;

    for(i = 127 ; i > - 1 ; i--) {
        for(j = 0 ; j < 128 ; j++) {
            int index = 0;
            rgb = readRGB(fr, transparency);
            minecraftRGB = selectRGB(rgb, &index, colorKey);
            grid[i][j] = index;
            fillRectangle(hdc, RGB(rgb.r, rgb.g, rgb.b), j*scale, (i - 1) * scale, (j + 1) * scale, i*scale);
            fillRectangle(hdc, RGB(minecraftRGB.r, minecraftRGB.g, minecraftRGB.b), (j + 128) * scale, (i - 1) * scale, (j + 1 + 128) * scale, i*scale);
            fprintf(colorIndices, "%i\n", index);
        }
    }

    fclose(fr);
    fclose(colorKey);
    fclose(colorIndices);
    q = buildQuadOG(grid, 128);
    generateCommands(q, colors);
    destroyQuad(&q);
    for(i = 0 ; i < 128 ; i++)
        free(grid[i]);
    free(grid);
    for(i = 0 ; colors[i] != NULL ; i++)
        free(colors[i]);
    free(colors);
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
    //inputTest();

    MSG msg = {};
    msg.hwnd = hwnd;
    while(GetMessage(&msg, 0, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
} 
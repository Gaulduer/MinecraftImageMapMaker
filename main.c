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
    const size_t len = 512 + 1;

    ip.type = INPUT_KEYBOARD;
    ip.ki.wScan = 0;
    ip.ki.time = 0;
    ip.ki.dwExtraInfo = 0;

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

        Sleep(70);

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

void quad(struct Quad *q, struct LinkedList *queue, int limit) {
    int i;
    if(q->size <= limit || q->leaf) {
        LL_append(queue, allocQuadMarker(q));
        return;
    }

    for(i = 0 ; i < 4 ; i++)
        quad(q->children[i], queue, limit);

    return;
}

void prioritizeMarkers(struct LinkedList *priorityQueue, int *counters, int colors) {
    int i;
    for(i = 0 ; i < colors ; i++)
        if(counters[i] > 0)
            if(LL_empty(priorityQueue) || counters[i] < counters[priorityQueue->tail->marker->colorKey])
                LL_append(priorityQueue, allocMarker(0, 0, 0, 0, i));
            else {
                struct Node *prev = NULL, *n = priorityQueue->head;
                while(n != NULL) {
                    prev = n;
                    n = n->next;
                }
                LL_insert(priorityQueue, prev, allocMarker(0, 0, 0, 0, i));
            }
}

void mergeColorCols(struct LinkedList *queue, struct LinkedList *lines, int c) {
    int i;
    for(i = 0 ; i < 128 ; i++) {
        struct Node *prev = NULL, *n = lines[i].head;
        int low = 0;

        while(n != NULL) {
            struct Marker *m1 = n->marker;
            if(m1->colorKey == c) {
                m1->low = low;
                LL_append(queue, LL_remove(&lines[i], prev, &n));
            }
            else {
                prev = n;
                n = n->next;
                continue;
            }
            while(n != NULL) {
                struct Marker *m2 = n->marker;
                if(m1->endCol + 1 == m2->startCol) {
                    m1->high = m2->endCol;
                    if(m2->colorKey == c) {
                        m1->endCol = m2->endCol;
                        free(LL_remove(&lines[i], prev, &n));
                    }
                    else {
                        prev = n;
                        n = n->next;
                    }
                }
                else {
                    low = m2->startCol;
                    break;
                }
            }
        }
    }
}

void mergeTwoRows(struct LinkedList *row1, struct LinkedList *row2) {
    struct Node *n1 = row1->head, *n2 = row2->head;

    while(n1 != NULL && n2 != NULL) {
        struct Marker *m1 = n1->marker, *m2 = n2->marker;
        if(m1->low <= m2->startCol && m1->high >= m2->endCol) {
            m1->endRow = m2->endRow;
            m1->low = m1->low > m2->low ? m1->low:m2->low;
            m1->startCol = m1->startCol < m2->startCol ? m1->startCol:m2->startCol;
            m1->high = m1->high < m2->high ? m1->high:m2->high;
            m1->endCol = m1->endCol > m2->endCol ? m1->endCol:m2->endCol;
            free(m2);
            n2->marker = m1;
            n2 = n2->next;
        }
        else if(m2->startCol > m1->high)
            n1 = n1->next;
        else
            n2 = n2->next;
    }
}

void mergeColorRows(struct LinkedList *queue, struct LinkedList *lines, int c) {
    int i, r1 = 0, r2;

    while(!LL_empty(queue) && queue->head->marker->colorKey) {
        i = queue->head->marker->startRow;
        printf("Placement: %i\n", i);
        LL_append(&lines[i], LL_removeHead(queue));
    }

    while(r1 < 128) {
        struct LinkedList *row1, *row2;

        if(LL_empty(&lines[r1])) {
            r1++;
            continue;
        };

        r2 = r1 + 1;

        while(r2 < 128 && LL_empty(&lines[r2]))
            r2++;

        if(r2 >= 128)
            break;

        row1 = &lines[r1];
        row2 = &lines[r2];

        printf("R1: %i, R2: %i\n", r1, r2);

        mergeTwoRows(row1, row2);

        r1 = r2;
    }

    for(i = 0 ; i < 128 ; i++)
        while(!LL_empty(&lines[i]))
            if(lines[i].head->marker->neuter) {
                LL_removeHead(&lines[i]);
            }
            else {
                lines[i].head->marker->neuter = 1;
                LL_append(queue, LL_removeHead(&lines[i]));
            }
}

void optimizeCommands(struct LinkedList *queue, int colors) {
    int i, *counters = malloc(colors * sizeof(int));
    struct LinkedList priorityQueue = {NULL, NULL}, lines[128];
    struct Node *priority; 

    for(i = 0 ; i < colors ; i++)
        counters[i] = 0;

    for(i = 0 ; i < 128 ; i++) {
        lines[i].head = NULL;
        lines[i].tail = NULL;
    }

    while(!LL_empty(queue)) {
        struct Marker *m = queue->head->marker;
        int placement = m->startRow;
        counters[m->colorKey]++;
        LL_append(&lines[placement], LL_removeHead(queue));
    }

    prioritizeMarkers(&priorityQueue, counters, colors);

    priority = priorityQueue.head;
    while(priority != NULL) {
        mergeColorCols(queue, lines, priority->marker->colorKey);
        priority = priority->next;
    }

    while(!LL_empty(&priorityQueue)) {
        //mergeColorRows(queue, lines, priorityQueue.head->marker->colorKey);
        free(LL_removeHead(&priorityQueue));
    }

    free(counters);
}

void generateCommands(struct Quad q, char **colors) {
    FILE *commands = fopen("commands.txt", "w");
    struct LinkedList commandQueue = {NULL, NULL};
    int i, n = 0;

    while(colors[i++] != NULL);
    n = i - 1;

    quad(&q, &commandQueue, 32);
    optimizeCommands(&commandQueue, n);
    while(!LL_empty(&commandQueue)) {
        writeCommand(commands, commandQueue.head->marker, colors);
        free(LL_removeHead(&commandQueue));
    }
    fclose(commands);
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
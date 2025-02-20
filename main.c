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
20241109 - The pixels shown for errors now get highlighted cyan if they are correct, magenta if they are wrong.
20241113 - There is are new commands to merge markers. They do not work, oops!
20241120 - Changed the prioritization method to consider 'breaks' in color rather than just counts of colors.
20241123 - Restructuring of project to later help with combo boxes. Places images and colorKey files into their respective folders.
20241125 - Combo boxes successfully added to allow for selection of preset images and color keys.
20241127 - Message loop altered. Set up before would cause the window to crash when interacted with while inputting commands.
New set up does not cause a crash when interacting with the window, as commands are run inside the message loop.
20241204 - Image change is now triggered within the message loop and not the window procedure. 
New combo box added for level of detail.
20241209 - Begining to make UI components and window scalable.
20250218 - Made the window's UI components scale when resized.
*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "bmp.h"
#include "linkedList.h"
#include "quad.h"
#include "windowUtil.h"
#include "display.h"

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

void simulateCommand(HDC hdc, struct Marker *m, uint32_t color, int xOffset, int yOffset, int scale) {
    uint32_t *pixels = allocSolidColor(m->endCol - m->startCol + 1, m->endRow - m->startRow + 1, color);
    fillRectangle(hdc, pixels, m->startCol + xOffset, m->startRow + yOffset, m->endCol + 1 + xOffset, m->endRow + 1 + yOffset, scale);
    free(pixels);
}

void clearDisplay(HDC hdc, int scale) {
    uint32_t *pixels = allocSolidColor(128, 128, 0x0FF00FF);
    fillRectangle(hdc, pixels, 128, 0, 256, 128, scale);
    free(pixels);
}

int progressCommands(HDC hdc, HWND selectedWindow, FILE *commands, FILE *colors, int scale) {
    struct Marker m;
    uint32_t color;
    char command[512];

    if(!fgets(command, 512, commands))
        return 0;

    fscanf(colors, "%u", &color);
    sscanf(command, "/fill ~%i ~-1 ~%i ~%i ~-1 ~%i", &m.startCol, &m.startRow, &m.endCol, &m.endRow);
    inputCommand(selectedWindow, command);
    simulateCommand(hdc, &m, color, 128, 0, scale);

    return 1;
}

HWND selectWindow() {
    HWND selectedWindow;
    WCHAR name[128];
    selectedWindow = GetForegroundWindow();
    GetWindowTextW(selectedWindow, name, 128);
    printf("Selected Window - %ls\n", name);
    return selectedWindow;
}

void imprintGrid(struct LinkedList *queue, int **grid) {
    int i, j;
    struct Node *n = queue->head;

    /* Since not all the indices may be filled, filling them with a default. */
    for(i = 0 ; i < 128 ; i++) {
        for(j = 0 ; j < 128 ; j++)
            grid[i][j] = -1; /* Setting to negative 1 since that is an impossible index. */
    }
    
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
    int line = 0, index = 0, diff, diffBuffer, r, g, b;
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

void quad(struct Quad *q, struct LinkedList *queue, int *layers, int limit, int layer) {
    int i;
    if(q->size <= limit || q->leaf) {
        LL_append(queue, allocQuadMarker(q));
        return;
    }

    for(i = 0 ; i < 4 ; i++)
        quad(q->children[i], queue, layers, limit, layer + 1);
}

void prioritizeMarkers(struct LinkedList *pq, int *counters, int n) {
    int i;
    for(i = 0 ; i < n ; i++) {
        if(counters[i] == 0)
            continue;
        if(LL_empty(pq) || counters[pq->tail->marker->colorKey] >= counters[i])
            LL_append(pq, allocMarker(0, 0, 0, 0, i));
        else {
            struct Node *prev = NULL, *n = pq->head;
            while(counters[n->marker->colorKey] >= counters[i]) {
                prev = n;
                n = n->next;
            }
            LL_insert(pq, prev, allocMarker(0, 0, 0, 0, i));
        }
    }
}

void extractColor(struct LinkedList (*lines)[128], int c) { /* Merges markers of color c in a single row together horizontally. */
    int i;
    for(i = 0 ; i < 128 ; i++) {
        int low = 0;
        struct Node *prev = NULL, *n = lines[0][i].head;
        while(n != NULL) {
            struct Marker *m1 = cloneMarker(n->marker); /* Starting by copy a valid marker. Any existing marker is of equal or lower priority */
            LL_append(&lines[1][i], m1); /* Adding the marker to the separate line set exclusive to the color 'c' */
            if(n->marker->colorKey == c) /* Taking the place of the marker with the same color. */
                free(LL_remove(&lines[0][i], prev, &n));
            else { /* Setting the marker up to be a filler marker in case no 'real' marker is found. */
                m1->colorKey = c; /* Setting the filler markers color key to the current color being extracted. */
                m1->neuter = 1; /* This is set in case no instance of the color is encountered on this row. */
                prev = n;
                n = n->next;
            }
            while(n != NULL) {
                struct Marker *m2 = n->marker;
                if(m1->high + 1 == m2->startCol) {
                    m1->high = m2->endCol;
                    if(m2->colorKey == c) {
                        if(m1->neuter) {
                            m1->neuter = 0; /* Turn the filler marker into a 'real' marker. */
                            m1->startCol = m2->startCol; /* Setting the 'startCol' (we want to keep 'low') */
                        }
                        m1->endCol = m2->endCol;
                        free(LL_remove(&lines[0][i], prev, &n));
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
}

int mergeMarker(struct Marker *m1, struct Marker *m2) {
    if(m1->startCol < m2->low || m1->endCol > m2->high || m2->startCol < m1->low || m2->endCol > m1->high) /* The markers cannot fit into eachother. */
        return 0;

    m2->startRow = m1->startRow; /* The startRow will always be taken from m1. */
    m2->neuter = m1->neuter; /* This should always be 0 if a merge occurs, but I am doing this just in case. */
   
    m2->startCol = m1->startCol < m2->startCol ? m1->startCol:m2->startCol;
    m2->endCol = m1->endCol > m2->endCol ? m1->endCol:m2->endCol;
    m2->low = m1->low > m2->low ? m1->low:m2->low;
    m2->high = m1->high < m2->high ? m1->high:m2->high;

    return 1;
}

void mergeColor(struct LinkedList *q, struct LinkedList *lines /* This is just the 'single color' set of lines. */, int detail) {
    int i;

    for(i = 0 ; i < 127 ; i++) {
        struct Node *prev = NULL, *n1 = lines[i].head, *n2 = lines[i + detail].head;
        while(n1 != NULL && n2 != NULL) {
            if(n1->marker->neuter) {
                prev = n1;
                n1 = n1->next;
                continue;
            }

            while(n2 != NULL && n2->marker->high < n1->marker->startCol)
                n2 = n2->next;

            if(n2 == NULL)
                break;

            if(mergeMarker(n1->marker, n2->marker))
                free(LL_remove(&lines[i], prev, &n1));
            else {
                prev = n1;
                n1 = n1->next;
            }
        }
    }

    for(i = 0 ; i < 128 ; i++) {
        while(!LL_empty(&lines[i]))
            if(lines[i].head->marker->neuter)
                free(LL_removeHead(&lines[i]));
            else
                LL_append(q, LL_removeHead(&lines[i]));
    }
}

void mergeCommands(struct LinkedList (*lines)[128], struct LinkedList *q, int colors, int detail) {
    int i, lastColor = -1;
    int *counters = malloc(colors * sizeof(int));
    struct LinkedList priorityQueue = {NULL, NULL};

    for(i = 0 ; i < colors ; i++)
        counters[i] = 0;

    while(!LL_empty(q)) {
        struct Marker *m = LL_removeHead(q);
        LL_append(&lines[0][m->startRow], m);
        LL_append(&lines[1][m->startCol], m);
    }

    for(i = 0 ; i < 128 ; i++) {
        int lastColor = -1;
        struct Node *n = lines[0][i].head;
        while(n != NULL) {
            if(lastColor != n->marker->colorKey) {
                counters[n->marker->colorKey]++;
                lastColor = n->marker->colorKey;
            }
            n = n->next;
        }
        lastColor = -1;
        while(!LL_empty(&lines[1][i])) {
            if(lastColor != lines[1][i].head->marker->colorKey) {
                counters[lines[1][i].head->marker->colorKey]++;
                lastColor = lines[1][i].head->marker->colorKey;
            }
            LL_removeHead(&lines[1][i]);
        }
    }

    prioritizeMarkers(&priorityQueue, counters, colors);

    while(!LL_empty(&priorityQueue)) {
        int c = priorityQueue.head->marker->colorKey;
        free(LL_removeHead(&priorityQueue));
        extractColor(lines, c);
        mergeColor(q, lines[1], detail);
    }
}

void optimizeCommands(struct LinkedList *queue, int colors, int detail) {
    int i;
    struct LinkedList priorityQueue = {NULL, NULL}, lines[2][128];
    for(i = 0 ; i < 128 ; i++) {
        lines[0][i].head = NULL;
        lines[0][i].tail = NULL;
        lines[1][i].head = NULL;
        lines[1][i].tail = NULL;
    }

    mergeCommands(lines, queue, colors, detail);
}

void testCommands(HDC hdc, uint32_t *pixelKey, int **original, int **optimized, int scale, struct LinkedList *q) {
    int i, j, count = 1;
    struct Node *n = q->head;
    while(n != NULL) {
        int wrong = 0, p = 0;
        struct Marker *m = n->marker;
        uint32_t *pixels = allocSolidColor(m->endCol - m->startCol + 1, m->endRow - m->startRow + 1, 0x0000FFFF);
        for(i = m->endRow ; i >= m->startRow ; i--)
            for(j = m->startCol ; j <= m->endCol ; j++) {
                if(original[i][j] != optimized[i][j] && optimized[i][j] == m->colorKey) {
                    wrong = 1;
                    pixels[p] = 0x00FF00FF; /* Marking incorrect pixels as magenta. */
                }
                p++;
            }

        if(wrong) {
            fillRectangle(hdc, pixels, m->startCol + 128, m->startRow, m->endCol + 1 + 128, m->endRow + 1, scale); /* Fill the rectangle back in to hide outline. */    
            printf("ERROR: %i, Start: (%i, %i), Color: %i\n", count, m->startCol, m->startRow, m->colorKey);
            getc(stdin); /* Pause for viewing */
        }
        free(pixels);

        pixels = allocSolidColor(m->endCol - m->startCol + 1, m->endRow - m->startRow + 1, pixelKey[m->colorKey]);
        fillRectangle(hdc, pixels, m->startCol + 128, m->startRow, m->endCol + 1 + 128, m->endRow + 1, scale); /* Fill the rectangle back in to hide outline. */
        free(pixels);

        n = n->next;
        count++; /* Keeps track of which marker we are on. */
    }
}

void generateCommands(struct Quad q, char **colors, uint32_t *pixelKey, int detail, int scale, HDC hdc) {
    FILE *commands = fopen(".\\commands.txt", "w"), *pixelColors = fopen(".\\pixelColors.txt", "w");
    struct LinkedList commandQueue = {NULL, NULL};
    int i, n = 0, *layers, **originalGrid = allocGrid(), **optimizedGrid = allocGrid();

    while(colors[i++] != NULL);
    n = i - 1;

    layers = malloc(n * sizeof(int));

    for(i = 0 ; i < n ; i++)
        layers[i] = __INT_MAX__;

    quad(&q, &commandQueue, layers, detail, 0);
    imprintGrid(&commandQueue, originalGrid);
    optimizeCommands(&commandQueue, n, detail);
    imprintGrid(&commandQueue, optimizedGrid);
    testCommands(hdc, pixelKey, originalGrid, optimizedGrid, scale, &commandQueue);

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

void readImage(HDC hdc, int scale, int detail, char *image, char *key) {
    FILE *fr = fopen(image, "rb");
    FILE *colorKey = fopen(key, "r");
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

    fillRectangle(hdc, pixels, 0, 0, 128, 128, scale);

    for(i = 0 ; i < 128 ; i++) {
        for(j = 0 ; j < 128 ; j++)
            grid[i][j] = selectColorIndex(pixelToRGB(pixels[(127 - i) * 128 + j]), colorKey);
    }

    q = buildQuadOG(grid, 128);
    generateCommands(q, colors, pixelKey, detail, scale, hdc);
    destroyQuad(&q);
    freeGrid(grid);
    for(i = 0 ; colors[i] != NULL ; i++)
        free(colors[i]);
    free(colors);
    free(pixels);
    free(pixelKey);
    fclose(fr);
    fclose(colorKey);
}

void fillComboBox(HWND comboBox, char *fileName) {
    int i;
    WCHAR buffer[128];
    FILE *csv = fopen(fileName, "r");

    if(csv == NULL) {
        printf("%s failed to open.\n", fileName);
        return;
    }

    while(fscanf(csv, "%ls", buffer) != EOF)
        SendMessageW(comboBox, CB_ADDSTRING, (WPARAM)0, (LPARAM)buffer);

    SendMessageW(comboBox, CB_SETCURSEL, (WPARAM)0, 0);
}

void getComboBoxText(HWND combo, char *dest) {
    int index;
    WCHAR buffer[128];
    index = SendMessageW(combo, CB_GETCURSEL, 0, 0);
    SendMessageW(combo, CB_GETLBTEXT, (WPARAM)index, (LPARAM)buffer);
    sprintf(dest, "%ls", buffer);
}

void imageChange(HWND parent) {
    char image[128] = "images\\", colorKey[128] = "colorKeys\\", detailBuffer[4];
    int detail = 0;
    HWND comboHolder = FindWindowExW(parent, NULL, NULL, NULL), combo = FindWindowExW(comboHolder, NULL, NULL, NULL), display = FindWindowExW(parent, comboHolder, NULL, NULL);
    getComboBoxText(combo, image + strlen(image));
    combo = FindWindowExW(comboHolder, combo, NULL, NULL);
    getComboBoxText(combo, colorKey + strlen(colorKey));
    combo = FindWindowExW(comboHolder, combo, NULL, NULL);
    getComboBoxText(combo, detailBuffer);
    sscanf(detailBuffer, "%i", &detail);
    readImage(getDisplayDC(display), getDisplayScale(display), detail, image, colorKey);
}

LRESULT CALLBACK ContainerProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch(msg) {
        case WM_CREATE:
            break;
        case WM_CLOSE:
            PostMessageW(hwnd, WM_QUIT, 0, 0);
            break;
        case WM_COMMAND: {
            SendMessageW(GetParent(hwnd), msg, wp, lp); // The window simply sends the command upstream to its parent.
        } break;
        case WM_SIZE: {

        } break;
        default: 
            return DefWindowProcW(hwnd, msg, wp, lp);
    }
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch(msg) {
        case WM_CREATE: {
            HWND comboHolder = CreateWindowW(L"container", L"ComboHolder", WS_VISIBLE | WS_OVERLAPPED | WS_CHILD, 0, 0, 512, 50, hwnd, NULL, NULL, NULL);
            HWND combo = CreateWindowW(L"ComboBox", L"Select Image", CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE, 0, 0, 512, 100, comboHolder, NULL, NULL, NULL);
            CreateWindowW(L"display", L"", WS_VISIBLE | WS_CHILD, 0, 0, 512, 256, hwnd, NULL, NULL, NULL);
            CreateWindowW(L"ComboBox", L"Select Color Key", CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE, 0, 25, 256, 100, comboHolder, NULL, NULL, NULL);
            CreateWindowW(L"ComboBox", L"Select Detail", CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE, 256, 25, 256, 100, comboHolder, NULL, NULL, NULL);
            CreateWindowW(L"Button", L"Begin", WS_VISIBLE | WS_OVERLAPPED | WS_CHILD , 0, 0, 0, 0, hwnd, (HMENU)0, NULL, NULL);
            fillComboBox(combo, "images\\images.csv");
            combo = FindWindowExW(comboHolder, combo, NULL, NULL);
            fillComboBox(combo, "colorKeys\\colorKeys.csv");
            combo = FindWindowExW(comboHolder, combo, NULL, NULL);
            fillComboBox(combo, "sizes.csv");
        } break;
        case WM_CLOSE:
            PostMessageW(hwnd, WM_QUIT, 0, 0);
            break;
        case WM_COMMAND: {
            switch(HIWORD(wp)) {
                case 0: {
                    switch(wp) {
                        case 0: { /* This posts a message so the message loop will see it. */
                            PostMessageW(hwnd, WM_NULL, 1, 0);
                        } break;
                        default:
                            printf("No command associated with WP %i\n", wp);
                    }
                } break;
                case CBN_SELCHANGE: {
                    PostMessageW(hwnd, WM_NULL, 0, 0);
                } break;
            }
        } break;
        case WM_SIZE: {
            WCHAR buffer[128];
            HWND combos = FindWindowExW(hwnd, NULL, NULL, NULL), display = FindWindowExW(hwnd, combos, NULL, NULL), button = FindWindowExW(hwnd, NULL, L"Button", NULL);
            HWND combo = FindWindowExW(combos, NULL, NULL, NULL);
            proportionalPosition(hwnd, display, 0.0, 0.0, 1.0, 0.7, 0.0, 0.0);
            proportionalPosition(hwnd, combos, 0.0, 0.7, 1.0, -1.0, 0.0, 0.0);
            proportionalPosition(combos, combo, 0.0, 0.0, 1.0, -1.0, 0.0, 0.0);
            combo = FindWindowExW(combos, combo, NULL, NULL);
            proportionalPosition(combos, combo, 0.0, 0.5, 0.5, -1.0, 0.0, 0.0);
            combo = FindWindowExW(combos, combo, NULL, NULL);
            proportionalPosition(combos, combo, 0.5, 0.5, 0.5, -1.0, 0.0, 0.0);
            proportionalPosition(hwnd, button, 0.0, 1.0, 1.0, 0.1, 0.0, -1.0);
            anchorPosition(hwnd, button, combos, 0, 2, 1);
            if(wp == 2)
                imageChange(hwnd);
        } break;
        case WM_EXITSIZEMOVE: {
            PostMessageW(hwnd, WM_NULL, 0, 0);
        } break;
        default: 
            return DefWindowProcW(hwnd, msg, wp, lp);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmd, int cmdShow) {
    int phase = 0;
    FILE *commands = NULL, *colors = NULL;
    HWND selectedWindow;
    WNDCLASSW wc = {}, containerClass = {}, displayClass = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"main";
    wc.lpszMenuName = L"Minecraft Image Map Maker";
    RegisterClassW(&wc);
    containerClass.lpfnWndProc = ContainerProc;
    containerClass.hInstance = hInstance;
    containerClass.lpszClassName = L"container";
    RegisterClassW(&containerClass);
    registerDisplayClass(&displayClass, hInstance);

    HWND hwnd = CreateWindowW(wc.lpszClassName, wc.lpszMenuName, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, 528, 512, NULL, NULL, NULL, NULL);
    HWND display = FindWindowExW(hwnd, NULL, L"display", NULL);

    MSG msg = {};
    msg.hwnd = hwnd;
    msg.message = WM_CREATE;
    while(msg.message != WM_QUIT) {
        if(PeekMessageW(&msg, hwnd, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if(phase == 0 && msg.message == WM_NULL && msg.wParam == 0)
            imageChange(hwnd);
        else if(phase == 0 && msg.message == WM_NULL && msg.wParam == 1) {
            phase = 1;
            printf("Select your window...\n");
            GetAsyncKeyState(VK_CONTROL); /* In case control was pressed before. */
        }
        else if(phase == 1 && GetAsyncKeyState(VK_CONTROL)) {
            phase = 2;
            selectedWindow = selectWindow();
            commands = fopen("commands.txt", "r");
            colors = fopen("pixelColors.txt", "r");
            clearDisplay(getDisplayDC(display), getDisplayScale(display));
        }
        else if(phase == 2 && !progressCommands(getDisplayDC(display), selectedWindow, commands, colors, getDisplayScale(display))){
            phase = 0;
            fclose(commands);
            fclose(colors);
            commands = NULL;
            colors = NULL;
        }
    }

    if(commands != NULL) {
        fclose(commands);
        fclose(colors);
    }

    DestroyWindow(hwnd);
    UnregisterClassW(L"main", hInstance);
    return 0;
} 
/*
Author: Peter Gauld
Project: Minecraft Map Image Maker.
File: windowUtil.c

Note: The purpose of this file is to help add useful functions to manipulate win32 windows.

Timeline:
20250217 - File created. Transfered functions over from main file.
*/

#include "windowUtil.h"

// Used to project a child onto its parent using a proportional limit. The float parameters are factors that help to scale the child.
void proportionalPosition(HWND parent, HWND child, float xFactor, float yFactor, float wFactor, float hFactor, float xOffset, float yOffset) {
    RECT rect;
    int wP, hP, wC, hC, x, y;

    GetClientRect(parent, &rect);

    wP = rect.right - rect.left;
    hP = rect.bottom - rect.top;

    rect = relativeRectangle(parent, child);
    
    if(wFactor < 0)
        wC = rect.right - rect.left;
    else
        wC = wP * wFactor;
    if(hFactor < 0)
        hC = rect.bottom - rect.top;
    else
        hC = hP * hFactor;
    x = wP * xFactor - (wC * xOffset);
    y = hP * yFactor + (hC * yOffset);

    SetWindowPos(child, parent, x, y, wC, hC, SWP_NOZORDER);
}

void pixelPlacement(HWND parent, HWND child, int x, int y, float xOffset, float yOffset) {
    RECT rect;
    int wC, hC;

    GetClientRect(parent, &rect);

    wC = rect.right - rect.left;
    hC = rect.bottom - rect.top;
    x = x - (wC * xOffset);
    y = y + (hC * yOffset);

    SetWindowPos(child, parent, x - (wC * xOffset), y, wC, hC, SWP_NOZORDER);
}

long* indexToEdge(RECT *r, int index) {
    //0:TOP, 1:RIGHT, 2:BOTTOM, 3:LEFT
    switch(index) {
        case 0: return &r->top;
        break;
        case 1: return &r->right;
        break;
        case 2: return &r->bottom;
        break;
        case 3: return &r->left;
        break;
    }

    return 0;
}

RECT relativeRectangle(HWND parent, HWND child) { // Provides a rectangle with points relative to the child's parent.
    RECT r;

    // Getting the rectangle, and obtaining the point for the position relative to the parent.
    GetClientRect(child, &r);
    MapWindowPoints(child, parent, (LPPOINT)&r, 2);

    return r;
}

HWND anchorPosition(HWND parent, HWND child, HWND anchor, int cEdge, int aEdge, int resize) { 
    RECT c = relativeRectangle(parent, child), a = relativeRectangle(parent, anchor);
    long *edge, width = c.right - c.left, height = c.bottom - c.top;

    edge = indexToEdge(&c, cEdge);
    *edge = *(indexToEdge(&a, aEdge));

    if(resize) { /* If resize is toggled, get the new size. */
        width = c.right - c.left;
        height = c.bottom - c.top;
    }
    
    SetWindowPos(child, parent, c.left, c.top, width, height, SWP_NOZORDER);
}

HWND getChildAfter(HWND parent, HWND child, int order) {
    int i;
    
    for(i = 0 ; i < order ; i++)
        child = FindWindowEx(parent, child, NULL, NULL);

    return child;
}

void fillComboBoxes(int comboBoxes) {

}

int numberOfChildren(HWND hwnd) {
    HWND child = FindWindowExW(hwnd, NULL, NULL, NULL);
    int c = 0;

    while(child != NULL) {
        c++;
        child = FindWindowExW(hwnd, child, NULL, NULL);
    }
}
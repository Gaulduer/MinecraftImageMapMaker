/*
Author: Peter Gauld
Project: containers
File: main.c

Timeline:
20241218 - File created.
*/

#include <windows.h>

void vBoxPlacement(HWND parent) {
    int y;
    HWND child = FindWindowExW(hwnd, NULL, NULL, NULL);
    while(child != NULL) {
        RECT r;
        GetClientRect(&r);
        SetWindowPos(child, parent, );
        child = FindWindowExW(parent, child, NULL, NULL);
    }
}
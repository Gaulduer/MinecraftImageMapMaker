/*
Author: Peter Gauld
Project: Minecraft Map Image Maker.
File: display.c

Note: This file is meant to implement a window class that can properly display the image and its expected output in minecraft.
This window offers padding to help maintain the aspect ratio of the canvas.

Timeline:
20250218 - File created.
*/

#include "display.h"
#include "windowUtil.h"

void sizeCanvas(HWND hwnd) {
    HWND canvas = FindWindowExW(hwnd, NULL, NULL, NULL);
    RECT rect;
    int wP, hP, wC, hC, base = 128;

    GetClientRect(hwnd, &rect);
    wP = (rect.right - rect.left);
    hP = (rect.bottom - rect.top);

    wC = wP/(2 * base); // The width needs to fit two 128:128 images side by side.
    hC = hP/base;

    base *= wC < hC ? wC:hC;

    if(base < 128) // The smallest dimension that can be displayed is 128
        base = 128;

    SetWindowPos(canvas, hwnd, 0, 0, 2 * base, base, SWP_NOZORDER);
    proportionalPosition(hwnd, canvas, 0.5, 1.0, -1.0, -1.0, 0.5, -1.0);
}

LRESULT CALLBACK DisplayProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch(msg) {
        case WM_CREATE: {
            CreateWindowW(L"Static", L"", WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
            sizeCanvas(hwnd);
        } break;
        case WM_CLOSE:
            PostMessageW(hwnd, WM_QUIT, 0, 0);
            break;
        case WM_SIZE: {
            sizeCanvas(hwnd);
        } break;
        default: 
            return DefWindowProcW(hwnd, msg, wp, lp);
    }
    return 0;
}

int getDisplayScale(HWND display) { // Obtaining the scale of the canvas.
    HWND canvas = FindWindowExW(display, NULL, NULL, NULL);
    RECT rect;
    GetClientRect(canvas, &rect);
    
    return (rect.bottom - rect.top)/128; // Taking the height since that will match the current scale when divited by 128.
}

HDC getDisplayDC(HWND display) {
    return GetDC(FindWindowEx(display, NULL, NULL, NULL));
}

void registerDisplayClass(WNDCLASSW *wc, HINSTANCE hInstance) {
    wc->lpfnWndProc = DisplayProc;
    wc->hInstance = hInstance;
    wc->lpszClassName = L"display";
    wc->hbrBackground = (HBRUSH)CreateSolidBrush(RGB(0, 0, 0));
    RegisterClassW(wc);
}
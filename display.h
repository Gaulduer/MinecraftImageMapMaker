/*
Author: Peter Gauld
Project: Minecraft Map Image Maker.
File: display.h

Timeline:
20250218 - File created.
*/

#include <windows.h>

void registerDisplayClass(WNDCLASSW*, HINSTANCE);
HDC getDisplayDC(HWND display);
int getDisplayScale(HWND display);
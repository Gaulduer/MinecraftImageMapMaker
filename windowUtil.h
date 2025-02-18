/*
Author: Peter Gauld
Project: Minecraft Map Image Maker.
File: windowUtil.h

Timeline:
20250217 - File created. Transfered functions over from main file.
*/

#include <windows.h>

void proportionalPosition(HWND parent, HWND child, float xFactor, float yFactor, float wFactor, float hFactor, float xOffset, float yOffset);
void pixelPlacement(HWND parent, HWND child, int x, int y, float xOffset, float yOffset);
long* indexToEdge(RECT *r, int index);
RECT relativeRectangle(HWND parent, HWND child);
HWND anchorPosition(HWND parent, HWND child, HWND anchor, int cEdge, int aEdge, int resize);
HWND getChildAfter(HWND parent, HWND child, int order);
int numberOfChildren(HWND hwnd);
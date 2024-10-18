/*
Author: Peter Gauld
Use: Header for bmp.c.
File: bmp.h

=== Timeline ===
20240801 - File created.
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

struct BMPHeader {
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
    uint32_t dibHeaderSize;
    int32_t width;
    int32_t height;
    uint16_t numPlanes;
    uint16_t bitsPerPixel;
    uint32_t compression;
    uint32_t imageSizeBytes;
    int32_t xResolutionPPM;
    int32_t yResolutionPPM;
    uint32_t numColors;
    uint32_t importantColors;
};

struct RGBColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

struct BMPHeader readBMPHeader(FILE*);
struct RGBColor readRGB(FILE*, int);
void printBMPHeader(struct BMPHeader);
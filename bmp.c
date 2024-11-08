/*
Author: Peter Gauld
Use: BMP Handler - Meant to help handle bmp files.
File: bmp.c

=== Timeline ===
20240801 - File created.
20240810 - Added option to handle images with transparency data.

=== Notes ===
Used this webpage for information and help on the BMP structure:
https://engineering.purdue.edu/ece264/16au/hw/HW13
*/

#include "bmp.h"

struct BMPHeader readBMPHeader(FILE *fr)  {
    struct BMPHeader h;

    fread(&h.type, sizeof(uint16_t), 1, fr);
    fread(&h.size, sizeof(uint32_t), 1, fr);
    fread(&h.reserved1, sizeof(uint16_t), 1, fr);
    fread(&h.reserved2, sizeof(uint16_t), 1, fr);
    fread(&h.offset, sizeof(uint32_t), 1, fr);
    fread(&h.dibHeaderSize, sizeof(uint32_t), 1, fr);
    fread(&h.width, sizeof(int32_t), 1, fr);
    fread(&h.height, sizeof(int32_t), 1, fr);
    fread(&h.numPlanes, sizeof(uint16_t), 1, fr);
    fread(&h.bitsPerPixel, sizeof(uint16_t), 1, fr);
    fread(&h.compression, sizeof(uint32_t), 1, fr);
    fread(&h.imageSizeBytes, sizeof(uint32_t), 1, fr);
    fread(&h.xResolutionPPM, sizeof(int32_t), 1, fr);
    fread(&h.yResolutionPPM, sizeof(int32_t), 1, fr);
    fread(&h.numColors, sizeof(uint32_t), 1, fr);
    fread(&h.importantColors, sizeof(uint32_t), 1, fr);

    return h;
}

struct RGBColor readRGB(FILE *fr, int transparency) {
    struct RGBColor rgb;
    fread(&rgb.b, sizeof(uint8_t), 1, fr);
    fread(&rgb.g, sizeof(uint8_t), 1, fr);
    fread(&rgb.r, sizeof(uint8_t), 1, fr);
    if(transparency)
        fread(&rgb.a, sizeof(uint8_t), 1, fr);
    return rgb;
}

uint32_t readPixel(FILE *fr, int transparency) {
    uint8_t toss;
    uint32_t pixel = 0;

    fread(&pixel, 3 * sizeof(uint8_t), 1, fr);
    if(transparency)
        fread(&toss, sizeof(uint8_t), 1, fr);

    return pixel;
}

struct RGBColor pixelToRGB(uint32_t pixel) {
    struct RGBColor rgb;
    rgb.r = (uint8_t)(pixel >> 16);
    rgb.g = (uint8_t)(pixel >> 8);
    rgb.b = (uint8_t)pixel;
    return rgb;
}

uint32_t rgbToPixel(struct RGBColor rgb) {
    uint32_t pixel = 0;

    pixel += (uint32_t)(rgb.r);
    pixel = pixel << 8;
    pixel += (uint32_t)(rgb.g);
    pixel = pixel << 8;
    pixel += (uint32_t)(rgb.b);

    return pixel;
}

void printBMPHeader(struct BMPHeader h) {
    printf("Type: %i\n", h.type);
    printf("Size: %i\n", h.size);
    printf("Reserved 1: %i\n", h.reserved1);
    printf("Reserved 2: %i\n", h.reserved2);
    printf("Offset: %i\n", h.offset);
    printf("DIB Header Size: %i\n", h.dibHeaderSize);
    printf("Width: %i\n", h.width);
    printf("Height: %i\n", h.height);
    printf("Num Planes: %i\n", h.numPlanes);
    printf("Bits Per Pixel: %i\n", h.bitsPerPixel);
    printf("Compression: %i\n", h.compression);
    printf("Image Size Bytes: %i\n", h.imageSizeBytes);
    printf("X Resolution PPM: %i\n", h.xResolutionPPM);
    printf("Y Resolution PPM: %i\n", h.yResolutionPPM);
    printf("Num Colors: %i\n", h.numColors);
    printf("Important Colors: %i\n", h.importantColors);
}
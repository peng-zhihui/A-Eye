#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#define BI_BITFIELDS 0x3

typedef struct tagBITMAPFILEHEADER
{
    uint16_t bfType;      /* file type, must be 0x4d42, ie. BM */
    uint32_t bfSize;      /* file size */
    uint16_t bfReserved1; /* reserved, must be 0 */
    uint16_t bfReserved2; /* reserved, must be 0 */
    uint32_t bfOffBits;   /* bmp data offset */
} __attribute__((packed)) BitMapFileHeader;

typedef struct tagBITMAPINFOHEADER
{
    uint32_t biSize;          /* size of the struct, must be 40 */
    uint32_t biWidth;         /* width in pixels */
    uint32_t biHeight;        /* height in pixels */
    uint16_t biPlanes;        /* must be 0 */
    uint16_t biBitCount;      /* bits of one pixel */
    uint32_t biCompression;   /* whether compressed or not */
    uint32_t biSizeImage;     /* bmp data size */
    uint32_t biXPelsPerMeter; /* width resolution in pixels */
    uint32_t biYPelsPerMeter; /* height resolution in pixels */
    uint32_t biClrUsed;       /* color number used */
    uint32_t biClrImportant;  /* important color number used */
} __attribute__((packed)) BitMapInfoHeader;

typedef struct tagRGBQUAD
{
    uint8_t rgbBlue;     /* blue component */
    uint8_t rgbGreen;    /* green component */
    uint8_t rgbRed;      /* red component */
    uint8_t rgbReserved; /* reserved */
} __attribute__((packed)) RgbQuad;

int rgb565tobmp(char *buf, int width, int height, const char *filename);

#ifdef __cplusplus
} // extern "C" {
#endif

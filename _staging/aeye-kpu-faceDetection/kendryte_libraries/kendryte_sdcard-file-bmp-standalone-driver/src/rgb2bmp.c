#include "ff.h"
#include "rgb2bmp.h"

int rgb565tobmp(char *buf, int width, int height, const char *filename)
{
    FIL file;
    FRESULT ret = FR_OK;
    uint32_t ret_len = 0;
    uint32_t i;
    uint16_t temp;
    uint16_t *ptr;

    BitMapFileHeader bmfHdr; /* file header */
    BitMapInfoHeader bmiHdr; /* information header */
    RgbQuad bmiClr[3];       /* color palette */

    bmiHdr.biSize = sizeof(BitMapInfoHeader);
    bmiHdr.biWidth = width;
    bmiHdr.biHeight = height;
    bmiHdr.biPlanes = 1;
    bmiHdr.biBitCount = 16;
    bmiHdr.biCompression = BI_BITFIELDS;
    bmiHdr.biSizeImage = (width * height * 2);
    bmiHdr.biXPelsPerMeter = 0;
    bmiHdr.biYPelsPerMeter = 0;
    bmiHdr.biClrUsed = 0;
    bmiHdr.biClrImportant = 0;

    /* rgb565 mask */
    bmiClr[0].rgbBlue = 0;
    bmiClr[0].rgbGreen = 0xF8;
    bmiClr[0].rgbRed = 0;
    bmiClr[0].rgbReserved = 0;

    bmiClr[1].rgbBlue = 0xE0;
    bmiClr[1].rgbGreen = 0x07;
    bmiClr[1].rgbRed = 0;
    bmiClr[1].rgbReserved = 0;

    bmiClr[2].rgbBlue = 0x1F;
    bmiClr[2].rgbGreen = 0;
    bmiClr[2].rgbRed = 0;
    bmiClr[2].rgbReserved = 0;

    bmfHdr.bfType = 0x4D42;
    bmfHdr.bfSize = sizeof(BitMapFileHeader) + sizeof(BitMapInfoHeader) + sizeof(RgbQuad) * 3 + bmiHdr.biSizeImage;
    bmfHdr.bfReserved1 = 0;
    bmfHdr.bfReserved2 = 0;
    bmfHdr.bfOffBits = sizeof(BitMapFileHeader) + sizeof(BitMapInfoHeader) + sizeof(RgbQuad) * 3;

    ptr = (uint16_t *)buf;

    for(i = 0; i < width * height; i += 2)
    {
        temp = ptr[i];
        ptr[i] = ptr[i + 1];
        ptr[i + 1] = temp;
    }

    if((ret = f_open(&file, filename, FA_CREATE_ALWAYS | FA_WRITE)) != FR_OK)
    {
        printf("create file %s err[%d]\n", filename, ret);
        return ret;
    }

    f_write(&file, &bmfHdr, sizeof(BitMapFileHeader), &ret_len);
    f_write(&file, &bmiHdr, sizeof(BitMapInfoHeader), &ret_len);
    f_write(&file, &bmiClr, 3 * sizeof(RgbQuad), &ret_len);

    f_write(&file, buf, bmiHdr.biSizeImage, &ret_len);

    f_close(&file);

    return 0;
}

#ifndef _OV5640_H
#define _OV5640_H

#include <stdint.h>

#define OV5640_ID 0X5640
#define OV5640_ADDR 0X78
#define OV5640_CHIPIDH 0X300A
#define OV5640_CHIPIDL 0X300B

#define XSIZE 320
#define YSIZE 240
#define LCD_GRAM_ADDRESS 0x60020000

#define QQVGA_160_120 0
#define QCIF_176_144 1
#define QVGA_320_240 2
#define WQVGA_400_240 3
#define CIF_352_288 4

#define jpeg_buf_size (30 * 1024)

uint8_t ov5640_init(void);
void ov5640_flash_lamp(uint8_t sw);

#endif

/* Copyright 2018 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* clang-format off */
#define LCD_X_MAX   (240)
#define LCD_Y_MAX   (320)

#define BLACK       0x0000
#define NAVY        0x000F
#define DARKGREEN   0x03E0
#define DARKCYAN    0x03EF
#define MAROON      0x7800
#define PURPLE      0x780F
#define OLIVE       0x7BE0
#define LIGHTGREY   0xC618
#define DARKGREY    0x7BEF
#define BLUE        0x001F
#define GREEN       0x07E0
#define CYAN        0x07FF
#define RED         0xF800
#define MAGENTA     0xF81F
#define YELLOW      0xFFE0
#define WHITE       0xFFFF
#define ORANGE      0xFD20
#define GREENYELLOW 0xAFE5
#define PINK        0xF81F
#define USER_COLOR  0xAA55
/* clang-format on */

typedef enum _lcd_dir {
  DIR_XY_RLUD = 0x00,
  DIR_YX_RLUD = 0x20,
  DIR_XY_LRUD = 0x40,
  DIR_YX_LRUD = 0x60,
  DIR_XY_RLDU = 0x80,
  DIR_YX_RLDU = 0xA0,
  DIR_XY_LRDU = 0xC0,
  DIR_YX_LRDU = 0xE0,
  DIR_XY_MASK = 0x20,
  DIR_MASK = 0xE0,
} lcd_dir_t;

typedef struct _lcd_ctl {
  uint8_t mode;
  uint8_t dir;
  uint16_t width;
  uint16_t height;
  uint16_t start_offset_w0;
  uint16_t start_offset_h0;
  uint16_t start_offset_w;
  uint16_t start_offset_h;
} lcd_ctl_t;

void set_area(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void set_direction(lcd_dir_t dir);
void enable_polling(void);
void enable_interrupt(void);

int get_width();
int get_height();

void clear_screen(uint16_t color);

void draw_point(uint16_t x, uint16_t y, uint16_t color);
void draw_char(uint16_t x, uint16_t y, char c, uint16_t color);
void draw_string(uint16_t x, uint16_t y, char *str, uint16_t color);

void fill_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                    uint16_t color);
void draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                    uint16_t width, uint16_t color);

void draw_picture(uint16_t x1, uint16_t y1, uint16_t width, uint16_t height,
                  uint32_t *ptr);
void draw_pic_roi(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t rx,
                  uint16_t ry, uint16_t rw, uint16_t rh, uint32_t *ptr);
void draw_picture_resized(uint16_t x1, uint16_t y1, uint16_t width,
                          uint16_t height, uint32_t *ptr);

int resize(void *ctx);

int init_st7789();

#ifdef __cplusplus
}  // extern "C" {
#endif

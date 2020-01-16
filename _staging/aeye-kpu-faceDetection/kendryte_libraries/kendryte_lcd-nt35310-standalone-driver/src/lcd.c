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
#include "lcd.h"
#include <string.h>
#include <unistd.h>
#include "font.h"
#include "nt35310.h"

lcd_ctl_t lcd_ctl;   // lcd params
uint16_t g_lcd_w;    // lcd width
uint16_t g_lcd_h;    // lcd height
uint8_t g_lcd_init;  // lcd init flag

uint16_t resized_frame[320 * 240];

void enable_polling(void) { lcd_ctl.mode = 0; }

void enable_interrupt(void) { lcd_ctl.mode = 1; }

int get_width() { return g_lcd_w; }

int get_height() { return g_lcd_h; }

void set_direction(lcd_dir_t dir) {
  if (!g_lcd_init) return;
  // dir |= 0x08; //excahnge RGB
  lcd_ctl.dir = dir;
  if (dir & DIR_XY_MASK) {
    lcd_ctl.width = g_lcd_w - 1;
    lcd_ctl.height = g_lcd_h - 1;
    lcd_ctl.start_offset_w = lcd_ctl.start_offset_h0;
    lcd_ctl.start_offset_h = lcd_ctl.start_offset_w0;
  } else {
    lcd_ctl.width = g_lcd_h - 1;
    lcd_ctl.height = g_lcd_w - 1;
    lcd_ctl.start_offset_w = lcd_ctl.start_offset_w0;
    lcd_ctl.start_offset_h = lcd_ctl.start_offset_h0;
  }

  tft_write_command(MEMORY_ACCESS_CTL);
  tft_write_byte((uint8_t *)&dir, 1);
}

void set_area(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
  uint8_t data[4] = {0};

  x1 += lcd_ctl.start_offset_w;
  x2 += lcd_ctl.start_offset_w;
  y1 += lcd_ctl.start_offset_h;
  y2 += lcd_ctl.start_offset_h;

  data[0] = (uint8_t)(x1 >> 8);
  data[1] = (uint8_t)(x1);
  data[2] = (uint8_t)(x2 >> 8);
  data[3] = (uint8_t)(x2);
  tft_write_command(HORIZONTAL_ADDRESS_SET);
  tft_write_byte(data, 4);

  data[0] = (uint8_t)(y1 >> 8);
  data[1] = (uint8_t)(y1);
  data[2] = (uint8_t)(y2 >> 8);
  data[3] = (uint8_t)(y2);
  tft_write_command(VERTICAL_ADDRESS_SET);
  tft_write_byte(data, 4);

  tft_write_command(MEMORY_WRITE);
}

void draw_point(uint16_t x, uint16_t y, uint16_t color) {
  set_area(x, y, x, y);
  //   tft_write_half((uint16_t *)&color, 1);
  uint8_t data[] = {color >> 8, color & 0xFF};
  tft_write_byte(data, 2);
}

void draw_char(uint16_t x, uint16_t y, char c, uint16_t color) {
  uint8_t i = 0;
  uint8_t j = 0;
  uint8_t data = 0;

  for (i = 0; i < 16; i++) {
    data = ascii0816[c * 16 + i];
    for (j = 0; j < 8; j++) {
      if (data & 0x80) draw_point(x + j, y, color);
      data <<= 1;
    }
    y++;
  }
}

void draw_string(uint16_t x, uint16_t y, char *str, uint16_t color) {
  while (*str) {
    draw_char(x, y, *str, color);
    str++;
    x += 8;
  }
}

void clear_screen(uint16_t color) {
  uint32_t data = ((uint32_t)color << 16) | (uint32_t)color;

  set_area(0, 0, lcd_ctl.width, lcd_ctl.height);
  tft_fill_data(&data, g_lcd_h * g_lcd_w / 2);
}

void fill_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                    uint16_t color) {
  if ((x1 == x2) || (y1 == y2)) return;
  uint32_t data = ((uint32_t)color << 16) | (uint32_t)color;
  set_area(x1, y1, x2 - 1, y2 - 1);
  tft_fill_data(&data, (x2 - x1) * (y2 - y1) / 2);
}

void draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                    uint16_t width, uint16_t color) {
  uint32_t data_buf[640] = {0};
  uint32_t *p = data_buf;
  uint32_t data = color;
  uint32_t index = 0;

  data = (data << 16) | data;
  for (index = 0; index < 160 * width; index++) *p++ = data;

  set_area(x1, y1, x2, y1 + width - 1);
  tft_write_byte((uint8_t *)data_buf, ((x2 - x1 + 1) * width + 1) * 2);
  set_area(x1, y2 - width + 1, x2, y2);
  tft_write_byte((uint8_t *)data_buf, ((x2 - x1 + 1) * width + 1) * 2);
  set_area(x1, y1, x1 + width - 1, y2);
  tft_write_byte((uint8_t *)data_buf, ((y2 - y1 + 1) * width + 1) * 2);
  set_area(x2 - width + 1, y1, x2, y2);
  tft_write_byte((uint8_t *)data_buf, ((y2 - y1 + 1) * width + 1) * 2);
}

void draw_picture_resized(uint16_t x1, uint16_t y1, uint16_t width,
                          uint16_t height, uint32_t *ptr) {
  uint16_t *p16 = (uint16_t *)ptr;

  float rw = 320 / width;
  float rh = 240 / height;

  for (float col = 0; col < width; col++)
    for (float row = 0; row < height; row++)
      resized_frame[(int)(row * width + col)] =
          *(p16 + (int)(roundf((row * 320 * rh) + col * rw)));

  set_area(x1, y1, x1 + width - 1, y1 + height - 1);
  tft_write_half(resized_frame, width * height);
}

// draw pic's roi on (x,y)
// x,y of LCD, w,h is pic; rx,ry,rw,rh is roi
void draw_pic_roi(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t rx,
                  uint16_t ry, uint16_t rw, uint16_t rh, uint32_t *ptr) {
  int y_oft;
  uint8_t *p;
  for (y_oft = 0; y_oft < rh; y_oft++) {  // draw line by line
    p = (uint8_t *)(ptr) + w * 2 * (y_oft + ry) + 2 * rx;
    set_area(x, y + y_oft, x + rw - 1, y + y_oft);
    tft_write_byte((uint8_t *)p, rw * 2);  //, ctl.mode ? 2 : 0);
  }
}

int init_st7789() {
  // Screen params
  lcd_dir_t direction = DIR_YX_RLDU;
  uint32_t freq = 30000000;
  lcd_ctl.start_offset_w0 = 52;
  lcd_ctl.start_offset_h0 = 40;
  g_lcd_w = 240;
  g_lcd_h = 135;

  uint8_t data = 0;

  tft_hard_init(freq, false);
  /*soft reset*/
  tft_write_command(SOFTWARE_RESET);
  msleep(150);
  /*exit sleep*/
  tft_write_command(SLEEP_OFF);
  msleep(500);
  /*pixel format*/

  tft_write_command(PIXEL_FORMAT_SET);
  data = 0x05;
  tft_write_byte(&data, 1);
  msleep(10);

  g_lcd_init = true;

  set_direction(direction);
  tft_write_command(INVERSION_DISPALY_ON);
  msleep(10);
  tft_write_command(NORMAL_DISPALY_ON);
  msleep(10);
  /*display on*/
  tft_write_command(DISPALY_ON);
  msleep(100);

  enable_polling();

  return 0;
}

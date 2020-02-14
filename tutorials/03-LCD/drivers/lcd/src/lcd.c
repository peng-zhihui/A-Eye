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
#include "sleep.h"
#include "st7789.h"

static lcd_ctl_t lcd_ctl;

static void lcd_backlight_init(void) {
  pwm_init(PWM_DEVICE_1);
  pwm_set_frequency(PWM_DEVICE_1, PWM_CHANNEL_0, 100000, 0.5);
  pwm_set_enable(PWM_DEVICE_1, PWM_CHANNEL_0, 1);
}

void lcd_polling_enable(void) { lcd_ctl.mode = 0; }

void lcd_interrupt_enable(void) { lcd_ctl.mode = 1; }

void lcd_init(void) {
  uint8_t data = 0;

  lcd_backlight_init();

  tft_hard_init();
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

  lcd_set_direction(DIR_YX_RLDU);
  tft_write_command(INVERSION_DISPALY_ON);
  msleep(10);
  tft_write_command(NORMAL_DISPALY_ON);
  msleep(10);

  /*display on*/
  tft_write_command(DISPALY_ON);
  msleep(100);
  lcd_polling_enable();
}

void lcd_set_direction(lcd_dir_t dir) {
  lcd_ctl.dir = dir;
  if (dir & DIR_XY_MASK) {
    lcd_ctl.width = LCD_Y_MAX - 1;
    lcd_ctl.height = LCD_X_MAX - 1;
    lcd_ctl.offset_h = LCD_X_OFFSET;
    lcd_ctl.offset_w = LCD_Y_OFFSET;
  } else {
    lcd_ctl.width = LCD_X_MAX - 1;
    lcd_ctl.height = LCD_Y_MAX - 1;
    lcd_ctl.offset_h = LCD_Y_OFFSET;
    lcd_ctl.offset_w = LCD_X_OFFSET;
  }

  tft_write_command(MEMORY_ACCESS_CTL);
  tft_write_byte((uint8_t *)&dir, 1);
}

void lcd_set_backlight(float percent) {
  pwm_set_frequency(PWM_DEVICE_1, PWM_CHANNEL_0, 100000, percent);
}

void lcd_set_area(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
  uint8_t data[4] = {0};

  x1 += lcd_ctl.offset_w;
  x2 += lcd_ctl.offset_w;
  y1 += lcd_ctl.offset_h;
  y2 += lcd_ctl.offset_h;

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

void lcd_draw_point(uint16_t x, uint16_t y, uint16_t color) {
  lcd_set_area(x, y, x, y);

  uint16_t temp = SWAP_16(color);
  tft_write_half(&temp, 1);
}

void lcd_draw_char(uint16_t x, uint16_t y, char c, uint16_t color) {
  uint8_t i = 0;
  uint8_t j = 0;
  uint8_t data = 0;

  for (i = 0; i < 16; i++) {
    data = ascii0816[c * 16 + i];
    for (j = 0; j < 8; j++) {
      if (data & 0x80) lcd_draw_point(x + j, y, color);
      data <<= 1;
    }
    y++;
  }
}

void lcd_draw_string(uint16_t x, uint16_t y, char *str, uint16_t color) {
  while (*str) {
    lcd_draw_char(x, y, *str, color);
    str++;
    x += 8;
  }
}

void lcd_ram_draw_string(char *str, uint32_t *ptr, uint16_t font_color,
                         uint16_t bg_color) {
  uint8_t i = 0;
  uint8_t j = 0;
  uint8_t data = 0;
  uint8_t *pdata = NULL;
  uint16_t width = 0;
  uint32_t *pixel = NULL;

  width = 4 * strlen(str);
  while (*str) {
    pdata = (uint8_t *)&ascii0816[(*str) * 16];
    for (i = 0; i < 16; i++) {
      data = *pdata++;
      pixel = ptr + i * width;
      for (j = 0; j < 4; j++) {
        switch (data >> 6) {
          case 0:
            *pixel = ((uint32_t)bg_color << 16) | bg_color;
            break;
          case 1:
            *pixel = ((uint32_t)bg_color << 16) | font_color;
            break;
          case 2:
            *pixel = ((uint32_t)font_color << 16) | bg_color;
            break;
          case 3:
            *pixel = ((uint32_t)font_color << 16) | font_color;
            break;
          default:
            *pixel = 0;
            break;
        }
        data <<= 2;
        pixel++;
      }
    }
    str++;
    ptr += 4;
  }
}

void lcd_clear(uint16_t color) {
  uint32_t data = ((uint32_t)color << 16) | (uint32_t)color;

  lcd_set_area(0, 0, lcd_ctl.width, lcd_ctl.height);
  tft_fill_data(&data, LCD_X_MAX * LCD_Y_MAX / 2);
}

void lcd_fill_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                        uint16_t color) {
  uint32_t data = color | (color << 16);

  if (x1 > x2) {
    uint16_t tmp = x1;
    x1 = x2;
    x2 = tmp;
  }
  if (y1 > y2) {
    uint16_t tmp = y1;
    y1 = y2;
    y2 = tmp;
  }
  if (x1 > LCD_X_MAX) x1 = LCD_X_MAX;
  if (x2 > LCD_X_MAX) x2 = LCD_X_MAX;
  if (y1 > LCD_Y_MAX) y1 = LCD_Y_MAX;
  if (y2 > LCD_Y_MAX) y2 = LCD_Y_MAX;

  int buff_size = 1 + (x2 - x1 + 1) * (y2 - y1 + 1) / 2;
  uint32_t data_buf[buff_size] __attribute__((aligned(64)));
  for (int index = 0; index < buff_size; index++) data_buf[index] = data;

  lcd_set_area(x1, y1, x2, y2);
  tft_write_word(data_buf, buff_size, 0);
}

void lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                        uint16_t width, uint16_t color) {
  uint32_t data_buf[640] = {0};
  uint32_t *p = data_buf;
  uint32_t data = color;
  uint32_t index = 0;

  data = (data << 16) | data;
  for (index = 0; index < 160 * width; index++) *p++ = data;

  lcd_set_area(x1, y1, x2, y1 + width - 1);
  tft_write_word(data_buf, ((x2 - x1 + 1) * width + 1) / 2, 0);
  lcd_set_area(x1, y2 - width + 1, x2, y2);
  tft_write_word(data_buf, ((x2 - x1 + 1) * width + 1) / 2, 0);
  lcd_set_area(x1, y1, x1 + width - 1, y2);
  tft_write_word(data_buf, ((y2 - y1 + 1) * width + 1) / 2, 0);
  lcd_set_area(x2 - width + 1, y1, x2, y2);
  tft_write_word(data_buf, ((y2 - y1 + 1) * width + 1) / 2, 0);
}

void lcd_draw_picture(uint16_t x1, uint16_t y1, uint16_t width, uint16_t height,
                      uint32_t *ptr) {
  lcd_set_area(x1, y1, x1 + width - 1, y1 + height - 1);
  tft_write_word(ptr, width * height / 2, lcd_ctl.mode ? 2 : 0);
}

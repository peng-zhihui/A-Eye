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
#include <fpioa.h>
#include <gpio.h>
#include <lcd.h>
#include <sleep.h>
#include <stdio.h>
#include <sysctl.h>
#include "A-Eye.h"

uint32_t g_lcd_gram[LCD_X_MAX * LCD_Y_MAX / 2] __attribute__((aligned(128)));

int main(void) {
  lcd_init();
  lcd_set_backlight(1);
  lcd_clear(WHITE);

  lcd_draw_picture(0, 0, A_EYE_IMAGE_WIDTH, A_EYE_IMAGE_HEIGHT, A_EYE_IMAGE);

  sleep(10);

  uint64_t time_start = sysctl_get_time_us();
  for (size_t i = 0; i < 1000; i++) {
    lcd_clear(WHITE);
  }
  uint64_t time_end = sysctl_get_time_us();

  uint16_t time_s = (time_end - time_start) / 1000000;

  char buffer[64];
  sprintf(buffer, "Total time: %ds  FPS: %dHz", time_s, 1000 / time_s);
  lcd_draw_string(15, 60, buffer, BLACK);

  while (1)
    ;
}

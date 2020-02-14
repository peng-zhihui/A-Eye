#include <ff.h>
#include <fpioa.h>
#include <gpio.h>
#include <lcd.h>
#include <sdcard.h>
#include <sleep.h>
#include <stdio.h>
#include <sysctl.h>
#include "A-Eye.h"
#include "spi.h"

FIL file;
FRESULT ret = FR_OK;
uint32_t v_ret_len = 0;
char filename_buffer[28];
uint8_t frame_data[64800] __attribute__((aligned(8)));

static int sdcard_init(void) {
  uint8_t status;

  status = sd_init();
  printf("sd init %d\n", status);
  if (status != 0) {
    return status;
  }

  printf("card info status %d\n", status);
  printf("CardCapacity:%ld\n", cardinfo.CardCapacity);
  printf("CardBlockSize:%d\n", cardinfo.CardBlockSize);
  return 0;
}

static int fs_init(void) {
  static FATFS sdcard_fs;
  FRESULT status;
  DIR dj;
  FILINFO fno;

  status = f_mount(&sdcard_fs, _T("0:"), 1);
  printf("mount sdcard:%d\n", status);
  if (status != FR_OK) return status;

  printf("printf filename\n");
  status = f_findfirst(&dj, &fno, _T("0:"), _T("*"));
  while (status == FR_OK && fno.fname[0]) {
    if (fno.fattrib & AM_DIR)
      printf("dir:%s\n", fno.fname);
    else
      printf("file:%s\n", fno.fname);
    status = f_findnext(&dj, &fno);
  }
  f_closedir(&dj);
  return 0;
}

int main(void) {
  sysctl_pll_set_freq(SYSCTL_PLL0, 800000000UL);
  sysctl_pll_set_freq(SYSCTL_PLL1, 800000000UL);

  plic_init();
  sysctl_enable_irq();
  spi_set_clk_rate(SPI_DEVICE_1, 30000000);
  sleep(1);
  if (sdcard_init()) {
    printf("SD card err\n");
    return -1;
  }
  if (fs_init()) {
    printf("FAT32 err\n");
    return -1;
  }

  lcd_init();
  lcd_set_backlight(0.2);
  lcd_clear(WHITE);
  lcd_draw_string(85, 65, "Bad Apple", BLACK);
  sleep(1);

  uint64_t time_start = sysctl_get_time_us();
  for (size_t i = 1; i < 1316; i++) {
    uint64_t draw_start_time = sysctl_get_time_us();
    sprintf(filename_buffer, "BadApple6PS/%d.bin", 61);
    if ((ret = f_open(&file, filename_buffer, FA_READ)) == FR_OK) {
      ret = f_read(&file, (void *)frame_data, 64800, &v_ret_len);
      f_close(&file);
    }

    lcd_draw_picture(0, 0, A_EYE_IMAGE_WIDTH, A_EYE_IMAGE_HEIGHT, frame_data);
    while ((sysctl_get_time_us() - draw_start_time) < 153000) {
      ;
    }
  }

  uint64_t time_end = sysctl_get_time_us();

  uint16_t time_s = (time_end - time_start) / 1000000;

  char buffer[64];
  lcd_clear(BLACK);
  lcd_draw_string(45, 40, "Bad Apple with K210", WHITE);
  sprintf(buffer, "Total time: %ds  FPS: %dHz", time_s, 1316 / time_s);
  lcd_draw_string(15, 90, buffer, WHITE);

  while (1)
    ;
}

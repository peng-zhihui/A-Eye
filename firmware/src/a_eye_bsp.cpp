#include "a_eye_bsp.h"

Screen lcd;
TFcard tfcard;
Camera camera;

void system_init()
{
  /* Set CPU and dvp clk */
  sysctl_pll_set_freq(SYSCTL_PLL0, 600000000UL); // core0
  sysctl_pll_set_freq(SYSCTL_PLL1, 600000000UL); // core1

  uarths_init();
  plic_init();

  /* LCD init */
  printf("LCD init...\n");
  lcd.init(ST7789_240x135_1d14);
  lcd.clear(WHITE);
  msleep(100);

  uint8_t res;

  /* TF-Card init */
  printf("TF-Card init...\n");
  res = tfcard.init();

  /* Camera init */
  printf("Camera init...\n");
  camera.init();

  /* enable global interrupt */
  sysctl_enable_irq();

  /* system start */
  printf("--- System start ---\n");
}

void delay(uint64_t msec) { msleep(msec); }

void run_on_core1(core_function func, void *ctx) { register_core1(func, ctx); }

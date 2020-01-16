#include "a_eye_bsp.h"


Screen lcd;
TFcard tfcard;
Camera camera;

void system_init()
{
  /* Set CPU and dvp clk */
  sysctl_pll_set_freq(SYSCTL_PLL0, 600000000UL); // core0
  sysctl_pll_set_freq(SYSCTL_PLL1, 400000000UL); // core1

  uarths_init();
  plic_init();

  /* user led */
  pwm_init(PWM_DEVICE_1);
  pwm_set_frequency(PWM_DEVICE_1, PWM_CHANNEL_1, 100000, 1);
  pwm_set_enable(PWM_DEVICE_1, PWM_CHANNEL_1, 1);

  /* LCD init */
  printf("LCD init...\n");
  lcd.init(ST7789_240x135_1d14);
  lcd.set_backlight(1);
  lcd.clear(BLACK);
  lcd.draw_string(0, 120, "Previewing @ 320x240", WHITE);
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

void start_core1(core_function func, void *ctx) { register_core1(func, ctx); }

void set_led(float percent) { pwm_set_frequency(PWM_DEVICE_1, PWM_CHANNEL_1, 100000, percent); }

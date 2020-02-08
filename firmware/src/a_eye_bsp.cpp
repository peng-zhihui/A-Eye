#include "a_eye_bsp.h"
#include "A_Eye.h"

Screen lcd;
TFcard tfcard;
Camera camera;
KPU nn;

void system_init() {
  /* Set CPU and dvp clk */
  sysctl_pll_set_freq(SYSCTL_PLL0, 600000000UL);  // core0
  sysctl_pll_set_freq(SYSCTL_PLL1, 400000000UL);  // core1
  sysctl_set_spi0_dvp_data(1);

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
  lcd.draw_picture(0, 0, A_EYE_IMAGE_WIDTH, A_EYE_IMAGE_HEIGHT,
                   (uint32_t *)A_EYE_IMAGE);
  sleep(10);
  lcd.clear(BLACK);
  lcd.draw_string(25, 120, "A-Eye with K210 @ 320x240", WHITE);
  msleep(100);

  /* Flash init */
  printf("flash init\n");
  w25qxx_init(3, 0, 40000000);
  w25qxx_read_data(KMODEL_START, nn.model, KMODEL_SIZE);

  /* TF-Card init */
  printf("TF-Card init...\n");
  tfcard.init();

  /* Camera init */
  printf("Camera init...\n");
  camera.init();

  /* KPU init */
  nn.init();

  /* enable global interrupt */
  sysctl_enable_irq();

  /* system start */
  printf("--- System start ---\n");
}

void delay(uint64_t msec) { msleep(msec); }

void start_core1(core_function func, void *ctx) { register_core1(func, ctx); }

void set_led(float percent) {
  pwm_set_frequency(PWM_DEVICE_1, PWM_CHANNEL_1, 50000, percent);
}

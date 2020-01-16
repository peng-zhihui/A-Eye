#include "ov_camera.h"

Camera::Camera() {}

Camera::~Camera() {}

volatile uint8_t Camera::g_ram_mux;
volatile uint8_t Camera::g_dvp_finish_flag;
uint32_t Camera::g_lcd_gram0[38400] __attribute__((aligned(64)));
uint32_t Camera::g_lcd_gram1[38400] __attribute__((aligned(64)));

void Camera::init() {
  camera_init();

  dvp_set_display_addr((long)Camera::g_lcd_gram0);
  dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 0);
  dvp_disable_auto();

  /* DVP interrupt config */
  printf("DVP interrupt config\n");
  plic_set_priority(IRQN_DVP_INTERRUPT, 1);
  plic_irq_register(IRQN_DVP_INTERRUPT, on_irq_dvp_ov, NULL);
  plic_irq_enable(IRQN_DVP_INTERRUPT);

  dvp_clear_interrupt(DVP_STS_FRAME_START | DVP_STS_FRAME_FINISH);
  dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 1);

  Camera::g_ram_mux = 0;
  Camera::g_dvp_finish_flag = 0;
}

void Camera::switch_gram() { Camera::g_dvp_finish_flag = 0; }

bool Camera::grab_frame_done() { return Camera::g_dvp_finish_flag; }

void Camera::save_picture() {
  rgb565tobmp((char *)(get_frame_ptr()), 320, 240, _T("0:photo.bmp"));

  printf("Saved picture to tfcard.\n");
}

uint32_t *Camera::get_frame_ptr() {
  return (Camera::g_ram_mux ? Camera::g_lcd_gram0 : Camera::g_lcd_gram1);
}

int on_irq_dvp_ov(void *ctx) {
  if (dvp_get_interrupt(DVP_STS_FRAME_FINISH)) {
    /* switch gram */
    dvp_set_display_addr(Camera::g_ram_mux ? (long)Camera::g_lcd_gram0
                                           : (long)Camera::g_lcd_gram1);

    dvp_clear_interrupt(DVP_STS_FRAME_FINISH);
    Camera::g_dvp_finish_flag = 1;
  } else {
    if (Camera::g_dvp_finish_flag == 0) dvp_start_convert();
    dvp_clear_interrupt(DVP_STS_FRAME_START);
  }

  return 0;
}

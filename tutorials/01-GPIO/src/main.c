#include <gpio.h>
#include <sleep.h>
#include "fpioa-config.h"

/*
 * LED connect with J12(IO number)
 * You can bind J12 pin with GPIO3 in device manager json
 */
int main(void) {
  gpio_init();
  gpio_set_drive_mode(3, GPIO_DM_OUTPUT);
  gpio_pin_value_t value = GPIO_PV_HIGH;
  gpio_set_pin(3, value);

  while (1) {
    gpio_set_pin(3, value = !value);
    sleep(1);
  }
}

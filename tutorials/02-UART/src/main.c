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
#include <stdio.h>
#include <string.h>
#include <sysctl.h>
#include <uart.h>

#define UART_NUM UART_DEVICE_1

int main() {
  plic_init();
  sysctl_enable_irq();

  uart_init(UART_NUM);
  uart_configure(UART_NUM, 115200, 8, UART_STOP_1, UART_PARITY_NONE);

  char *hel = {"hello world!\n"};
  uart_send_data(UART_NUM, hel, strlen(hel));

  char c;
  while (1) {
    if (uart_receive_data(UART_NUM, &c, 1)) {
      uart_send_data(UART_NUM, &c, 1);
    }
  }
}

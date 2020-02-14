#ifndef _A_EYE_BSP_H_
#define _A_EYE_BSP_H_

#include <bsp.h>
#include <camera.h>
#include <dvp.h>
#include <ff.h>
#include <fpioa.h>
#include <gpiohs.h>
#include <plic.h>

#include <sleep.h>
#include <spi.h>
#include <stdio.h>
#include <string.h>
#include <sysctl.h>
#include <uarths.h>
#include <unistd.h>

#include "kpu_wrapper.h"
#include "ov_camera.h"
#include "screen.h"
#include "tfcard.h"

#ifdef __cplusplus
extern "C" {
#endif

extern Screen lcd;
extern TFcard tfcard;
extern Camera camera;
extern KPU nn;

void delay(uint64_t msec);
void start_core1(core_function func, void *ctx);
int main_core1(void *ctx);

void system_init();
void set_led(float percent);

#ifdef __cplusplus
}
#endif

#endif

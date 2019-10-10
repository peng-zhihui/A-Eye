#include <dvp.h>
#include <fpioa.h>
#include <gpiohs.h>
#include <sysctl.h>
#include "ov5640.h"

static void io_set_power(void)
{
    /* Set dvp and spi pin to 1.8V */
  sysctl_set_power_mode(SYSCTL_POWER_BANK6, SYSCTL_POWER_V18);
  sysctl_set_power_mode(SYSCTL_POWER_BANK7, SYSCTL_POWER_V18);
}

void camera_init()
{
    // sysctl_set_spi0_dvp_data(1);
    gpiohs_set_drive_mode(8, GPIO_DM_INPUT);

    io_set_power();

    dvp_init(16);
    dvp_set_xclk_rate(12000000);
    dvp_enable_burst();
    dvp_set_output_enable(0, 1);
    dvp_set_output_enable(1, 1);
    dvp_set_image_format(DVP_CFG_RGB_FORMAT);
    dvp_set_image_size(320, 240);
    ov5640_init();
}

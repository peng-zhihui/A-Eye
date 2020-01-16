#include <fpioa.h>
#include "fpioa-config.h"

int ide_config_fpioa() {
int ret = 0;

ret += fpioa_set_function(22, FUNC_SPI0_SS3);
ret += fpioa_set_function(19, FUNC_SPI0_SCLK);
ret += fpioa_set_function(43, FUNC_CMOS_RST);
ret += fpioa_set_function(42, FUNC_CMOS_PWDN);
ret += fpioa_set_function(46, FUNC_CMOS_XCLK);
ret += fpioa_set_function(45, FUNC_CMOS_VSYNC);
ret += fpioa_set_function(47, FUNC_CMOS_HREF);
ret += fpioa_set_function(44, FUNC_CMOS_PCLK);
ret += fpioa_set_function(41, FUNC_SCCB_SCLK);
ret += fpioa_set_function(40, FUNC_SCCB_SDA);
ret += fpioa_set_function(21, FUNC_SPI0_D0);
ret += fpioa_set_function(28, FUNC_SPI1_D0);
ret += fpioa_set_function(29, FUNC_SPI1_D1);
ret += fpioa_set_function(27, FUNC_SPI1_SCLK);
ret += fpioa_set_function(26, FUNC_GPIOHS7);
ret += fpioa_set_function(18, FUNC_GPIOHS30);
ret += fpioa_set_function(20, FUNC_GPIOHS31);
ret += fpioa_set_function(23, FUNC_TIMER1_TOGGLE1);

return ret;
}

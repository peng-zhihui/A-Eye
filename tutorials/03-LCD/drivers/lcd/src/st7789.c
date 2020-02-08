#include "st7789.h"
#include "gpiohs.h"
#include "sleep.h"
#include "spi.h"
#include "sysctl.h"
#include "unistd.h"

static void init_dcx(void) {
  gpiohs_set_drive_mode(DCX_GPIONUM, GPIO_DM_OUTPUT);
  gpiohs_set_pin(DCX_GPIONUM, GPIO_PV_HIGH);
}

static void set_dcx_control(void) { gpiohs_set_pin(DCX_GPIONUM, GPIO_PV_LOW); }

static void set_dcx_data(void) { gpiohs_set_pin(DCX_GPIONUM, GPIO_PV_HIGH); }

static void init_rst(void) {
  gpiohs_set_drive_mode(RST_GPIONUM, GPIO_DM_OUTPUT);
  gpiohs_set_pin(RST_GPIONUM, GPIO_PV_HIGH);
}

static void set_rst(uint8_t value) { gpiohs_set_pin(DCX_GPIONUM, value); }

void tft_hard_init(void) {
  init_dcx();
  init_rst();
  set_rst(0);
  sysctl_set_power_mode(SYSCTL_POWER_BANK1, SYSCTL_POWER_V18);
  sysctl_set_spi0_dvp_data(1);
  spi_init(SPI_CHANNEL, SPI_WORK_MODE_0, SPI_FF_STANDARD, 8, 0);
  spi_set_clk_rate(SPI_CHANNEL, 30000000);
  msleep(50);
  set_rst(1);
  msleep(50);
}

void tft_write_command(uint8_t cmd) {
  set_dcx_control();
  spi_init(SPI_CHANNEL, SPI_WORK_MODE_0, SPI_FF_STANDARD, 8, 0);
  // spi_init_non_standard(SPI_CHANNEL, 8 /*instrction length*/,
  //                       0 /*address length*/, 0 /*wait cycles*/,
  //                       SPI_AITM_AS_FRAME_FORMAT /*spi address trans mode*/);
  spi_send_data_normal_dma(SPI_DMA_CHANNEL, SPI_CHANNEL, SPI_SLAVE_SELECT,
                           (uint8_t *)(&cmd), 1, SPI_TRANS_CHAR);
}

void tft_write_byte(uint8_t *data_buf, uint32_t length) {
  set_dcx_data();
  spi_init(SPI_CHANNEL, SPI_WORK_MODE_0, SPI_FF_STANDARD, 8, 0);
  // spi_init_non_standard(SPI_CHANNEL, 8 /*instrction length*/,
  //                       0 /*address length*/, 0 /*wait cycles*/,
  //                       SPI_AITM_AS_FRAME_FORMAT /*spi address trans mode*/);
  spi_send_data_normal_dma(SPI_DMA_CHANNEL, SPI_CHANNEL, SPI_SLAVE_SELECT,
                           data_buf, length, SPI_TRANS_CHAR);
}

void tft_write_half(uint16_t *data_buf, uint32_t length) {
  set_dcx_data();
  spi_init(SPI_CHANNEL, SPI_WORK_MODE_0, SPI_FF_STANDARD, 16, 0);
  // spi_init_non_standard(SPI_CHANNEL, 16 /*instrction length*/,
  //                       0 /*address length*/, 0 /*wait cycles*/,
  //                       SPI_AITM_AS_FRAME_FORMAT /*spi address trans mode*/);
  spi_send_data_normal_dma(SPI_DMA_CHANNEL, SPI_CHANNEL, SPI_SLAVE_SELECT,
                           data_buf, length, SPI_TRANS_SHORT);
}

void tft_write_word(uint32_t *data_buf, uint32_t length, uint32_t flag) {
  set_dcx_data();
  spi_init(SPI_CHANNEL, SPI_WORK_MODE_0, SPI_FF_STANDARD, 32, 0);

  // spi_init_non_standard(SPI_CHANNEL, 0 /*instrction length*/,
  //                       32 /*address length*/, 0 /*wait cycles*/,
  //                       SPI_AITM_AS_FRAME_FORMAT /*spi address trans mode*/);
  spi_send_data_normal_dma(SPI_DMA_CHANNEL, SPI_CHANNEL, SPI_SLAVE_SELECT,
                           data_buf, length, SPI_TRANS_INT);
}

void tft_fill_data(uint32_t *data_buf, uint32_t length) {
  set_dcx_data();
  spi_init(SPI_CHANNEL, SPI_WORK_MODE_0, SPI_FF_STANDARD, 32, 0);
  // spi_init_non_standard(SPI_CHANNEL, 0 /*instrction length*/,
  //                       32 /*address length*/, 0 /*wait cycles*/,
  //                       SPI_AITM_AS_FRAME_FORMAT /*spi address trans mode*/);
  spi_fill_data_dma(SPI_DMA_CHANNEL, SPI_CHANNEL, SPI_SLAVE_SELECT, data_buf,
                    length);
}

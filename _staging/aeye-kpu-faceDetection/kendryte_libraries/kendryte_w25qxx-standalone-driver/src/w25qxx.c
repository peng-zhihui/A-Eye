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
#include <dmac.h>
#include <fpioa.h>
#include <spi.h>
#include <stdio.h>
#include <string.h>
#include <sysctl.h>
#include "w25qxx.h"

uint32_t spi_bus_no = 0;
uint32_t spi_chip_select = 0;

static w25qxx_status_t w25qxx_receive_data(uint8_t *cmd_buff, uint8_t cmd_len, uint8_t *rx_buff, uint32_t rx_len)
{
    spi_init(spi_bus_no, SPI_WORK_MODE_0, SPI_FF_STANDARD, DATALENGTH, 0);
    spi_receive_data_standard(spi_bus_no, spi_chip_select, cmd_buff, cmd_len, rx_buff, rx_len);
    return W25QXX_OK;
}

static w25qxx_status_t w25qxx_send_data(uint8_t *cmd_buff, uint8_t cmd_len, uint8_t *tx_buff, uint32_t tx_len)
{
    spi_init(spi_bus_no, SPI_WORK_MODE_0, SPI_FF_STANDARD, DATALENGTH, 0);
    spi_send_data_standard(spi_bus_no, spi_chip_select, cmd_buff, cmd_len, tx_buff, tx_len);
    return W25QXX_OK;
}

static w25qxx_status_t w25qxx_receive_data_enhanced_dma(uint32_t *cmd_buff, uint8_t cmd_len, uint8_t *rx_buff, uint32_t rx_len)
{
    spi_receive_data_multiple_dma(DMAC_CHANNEL0, DMAC_CHANNEL1, spi_bus_no, spi_chip_select, cmd_buff, cmd_len, rx_buff, rx_len);
    return W25QXX_OK;
}

static w25qxx_status_t w25qxx_send_data_enhanced_dma(uint32_t *cmd_buff, uint8_t cmd_len, uint8_t *tx_buff, uint32_t tx_len)
{
    spi_send_data_multiple_dma(DMAC_CHANNEL0, spi_bus_no, spi_chip_select, cmd_buff, cmd_len, tx_buff, tx_len);
    return W25QXX_OK;
}

w25qxx_status_t w25qxx_read_id(uint8_t *manuf_id, uint8_t *device_id)
{
    uint8_t cmd[4] = {READ_ID, 0x00, 0x00, 0x00};
    uint8_t data[2] = {0};

    w25qxx_receive_data(cmd, 4, data, 2);
    *manuf_id = data[0];
    *device_id = data[1];
    return W25QXX_OK;
}

static w25qxx_status_t w25qxx_write_enable(void)
{
    uint8_t cmd[1] = {WRITE_ENABLE};

    w25qxx_send_data(cmd, 1, 0, 0);
    return W25QXX_OK;
}

static w25qxx_status_t w25qxx_write_status_reg(uint8_t reg1_data, uint8_t reg2_data)
{
    uint8_t cmd[3] = {WRITE_REG1, reg1_data, reg2_data};

    w25qxx_write_enable();
    w25qxx_send_data(cmd, 3, 0, 0);
    return W25QXX_OK;
}

static w25qxx_status_t w25qxx_read_status_reg1(uint8_t *reg_data)
{
    uint8_t cmd[1] = {READ_REG1};
    uint8_t data[1] = {0};

    w25qxx_receive_data(cmd, 1, data, 1);
    *reg_data = data[0];
    return W25QXX_OK;
}

static w25qxx_status_t w25qxx_read_status_reg2(uint8_t *reg_data)
{
    uint8_t cmd[1] = {READ_REG2};
    uint8_t data[1] = {0};

    w25qxx_receive_data(cmd, 1, data, 1);
    *reg_data = data[0];
    return W25QXX_OK;
}

static w25qxx_status_t w25qxx_enable_quad_mode(void)
{
    uint8_t reg_data = 0;

    w25qxx_read_status_reg2(&reg_data);
    if(!(reg_data & REG2_QUAL_MASK))
    {
        reg_data |= REG2_QUAL_MASK;
        w25qxx_write_status_reg(0x00, reg_data);
    }
    return W25QXX_OK;
}

static w25qxx_status_t w25qxx_is_busy(void)
{
    uint8_t status = 0;

    w25qxx_read_status_reg1(&status);
    if(status & REG1_BUSY_MASK)
        return W25QXX_BUSY;
    return W25QXX_OK;
}

w25qxx_status_t w25qxx_sector_erase(uint32_t addr)
{
    uint8_t cmd[4] = {SECTOR_ERASE};

    cmd[1] = (uint8_t)(addr >> 16);
    cmd[2] = (uint8_t)(addr >> 8);
    cmd[3] = (uint8_t)(addr);
    w25qxx_write_enable();
    w25qxx_send_data(cmd, 4, 0, 0);
    return W25QXX_OK;
}

static w25qxx_status_t w25qxx_quad_page_program(uint32_t addr, uint8_t *data_buf, uint32_t length)
{
    uint32_t cmd[2] = {0};

    cmd[0] = QUAD_PAGE_PROGRAM;
    cmd[1] = addr;
    w25qxx_write_enable();
    spi_init(spi_bus_no, SPI_WORK_MODE_0, SPI_FF_QUAD, 32 /*DATALENGTH*/, 0);
    spi_init_non_standard(spi_bus_no, 8 /*instrction length*/, 24 /*address length*/, 0 /*wait cycles*/,
                          SPI_AITM_STANDARD /*spi address trans mode*/);
    w25qxx_send_data_enhanced_dma(cmd, 2, data_buf, length);
    while(w25qxx_is_busy() == W25QXX_BUSY)
        ;
    return W25QXX_OK;
}

static w25qxx_status_t w25qxx_sector_program(uint32_t addr, uint8_t *data_buf)
{
    uint8_t index = 0;

    for(index = 0; index < w25qxx_FLASH_PAGE_NUM_PER_SECTOR; index++)
    {
        w25qxx_quad_page_program(addr, data_buf, w25qxx_FLASH_PAGE_SIZE);
        addr += w25qxx_FLASH_PAGE_SIZE;
        data_buf += w25qxx_FLASH_PAGE_SIZE;
    }
    return W25QXX_OK;
}

w25qxx_status_t w25qxx_write_data_direct(uint32_t addr, uint8_t *data_buf, uint32_t length)
{
    if ( length % w25qxx_FLASH_SECTOR_SIZE != 0 ) {
        return W25QXX_ERROR;
    }
    for(uint32_t i = 0 ; i < length ; i+= 0x010000) {
        w25qxx_sector_program(addr + i, data_buf + i);
    }
    return W25QXX_OK;
}

w25qxx_status_t w25qxx_write_data(uint32_t addr, uint8_t *data_buf, uint32_t length)
{
    uint32_t sector_addr = 0;
    uint32_t sector_offset = 0;
    uint32_t sector_remain = 0;
    uint32_t write_len = 0;
    uint32_t index = 0;
    uint8_t *pread = NULL;
    uint8_t *pwrite = NULL;
    uint8_t swap_buf[w25qxx_FLASH_SECTOR_SIZE] = {0};

    while(length)
    {
        sector_addr = addr & (~(w25qxx_FLASH_SECTOR_SIZE - 1));
        sector_offset = addr & (w25qxx_FLASH_SECTOR_SIZE - 1);
        sector_remain = w25qxx_FLASH_SECTOR_SIZE - sector_offset;
        write_len = ((length < sector_remain) ? length : sector_remain);
        w25qxx_read_data(sector_addr, swap_buf, w25qxx_FLASH_SECTOR_SIZE);
        pread = swap_buf + sector_offset;
        pwrite = data_buf;
        for(index = 0; index < write_len; index++)
        {
            if((*pwrite) != ((*pwrite) & (*pread)))
            {
                w25qxx_sector_erase(sector_addr);
                while(w25qxx_is_busy() == W25QXX_BUSY)
                    ;
                break;
            }
            pwrite++;
            pread++;
        }
        if(write_len == w25qxx_FLASH_SECTOR_SIZE)
        {
            w25qxx_sector_program(sector_addr, data_buf);
        } else
        {
            pread = swap_buf + sector_offset;
            pwrite = data_buf;
            for(index = 0; index < write_len; index++)
                *pread++ = *pwrite++;
            w25qxx_sector_program(sector_addr, swap_buf);
        }
        length -= write_len;
        addr += write_len;
        data_buf += write_len;
    }
    return W25QXX_OK;
}

static w25qxx_status_t _w25qxx_read_data(uint32_t addr, uint8_t *data_buf, uint32_t length)
{
    uint32_t cmd[2] = {0};
    cmd[0] = FAST_READ_QUAL_IO;
    cmd[1] = addr << 8;
    spi_init(spi_bus_no, SPI_WORK_MODE_0, SPI_FF_QUAD, 32, 0);
    spi_init_non_standard(spi_bus_no, 8 /*instrction length*/, 32 /*address length*/, 4 /*wait cycles*/,
                          SPI_AITM_ADDR_STANDARD /*spi address trans mode*/);
    w25qxx_receive_data_enhanced_dma(cmd, 2, data_buf, length);
    return W25QXX_OK;
}

w25qxx_status_t w25qxx_read_data(uint32_t addr, uint8_t *data_buf, uint32_t length)
{
    uint32_t v_remain = length % 4;
    if(v_remain != 0)
    {
        length = length / 4 * 4;
    }

    uint32_t len = 0;

    while(length)
    {
        len = ((length >= 0x010000) ? 0x010000 : length);
        _w25qxx_read_data(addr, data_buf, len);
        addr += len;
        data_buf += len;
        length -= len;
    }

    if(v_remain)
    {
        uint8_t v_recv_buf[4];
        _w25qxx_read_data(addr, v_recv_buf, 4);
        memcpy(data_buf, v_recv_buf, v_remain);
    }
    return W25QXX_OK;
}

w25qxx_status_t w25qxx_init(uint8_t spi_index, uint8_t spi_ss, uint32_t rate)
{
    spi_bus_no = spi_index;
    spi_chip_select = spi_ss;
    spi_init(spi_bus_no, SPI_WORK_MODE_0, SPI_FF_STANDARD, DATALENGTH, 0);
    spi_set_clk_rate(spi_bus_no, rate);
    w25qxx_enable_quad_mode();
    return W25QXX_OK;
}

/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "diskio.h" /* FatFs lower layer API */
#include "sdcard.h"

/* Definitions of physical drive number for each drive */
#define M0 0 /* Example: Map MMC/SD card to physical drive 0 */

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(BYTE pdrv)
{
    return 0;
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize(BYTE pdrv)
{
    if(sd_init() == 0)
        return 0;
    return STA_NOINIT;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count)
{
    if(sd_read_sector_dma(buff, sector, count) == 0)
        return RES_OK;
    return RES_ERROR;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count)
{
    if(sd_write_sector_dma((BYTE *)buff, sector, count) == 0)
        return RES_OK;
    return RES_ERROR;
}

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    DRESULT res = RES_ERROR;

    switch(cmd)
    {
        /* Make sure that no pending write process */
        case CTRL_SYNC:
            res = RES_OK;
            break;
        /* Get number of sectors on the disk (DWORD) */
        case GET_SECTOR_COUNT:
            *(DWORD *)buff = (cardinfo.SD_csd.DeviceSize + 1) << 10;
            res = RES_OK;
            break;
        /* Get R/W sector size (WORD) */
        case GET_SECTOR_SIZE:
            *(WORD *)buff = cardinfo.CardBlockSize;
            res = RES_OK;
            break;
        /* Get erase block size in unit of sector (DWORD) */
        case GET_BLOCK_SIZE:
            *(DWORD *)buff = cardinfo.CardBlockSize;
            res = RES_OK;
            break;
        default:
            res = RES_PARERR;
    }
    return res;
}

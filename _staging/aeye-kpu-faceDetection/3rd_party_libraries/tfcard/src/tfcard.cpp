#include "tfcard.h"

TFcard::TFcard()
{
}

TFcard::~TFcard()
{
}

int TFcard::init()
{
    return init(26);
}

int TFcard::init(int cs)
{
    // gpio config, defalut uses spi1/io26-cs
    fpioa_set_function(cs, FUNC_GPIOHS7);

    if (sd_init() != 0)
    {
        printf("Fail to init SD card\n");
        return -1;
    }

    printf("CardCapacity: %ld\n", cardinfo.CardCapacity);
    printf("CardBlockSize: %d\n", cardinfo.CardBlockSize);

    /* mount file system to SD card */
    if (f_mount(&sdcard_fs, _T("0:"), 1))
    {
        printf("Fail to mount file system\n");
        return -1;
    }
}

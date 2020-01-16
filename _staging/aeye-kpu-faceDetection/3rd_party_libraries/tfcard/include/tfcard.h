#ifndef _TFCARD_H_
#define _TFCARD_H_

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "sleep.h"
#include "stdlib.h"
#include "fpioa.h"
#include "spi.h"
#include <sdcard.h>
#include <ff.h>

#ifdef __cplusplus
extern "C"
{
#endif

class TFcard
{
private:
  FATFS sdcard_fs;

public:
    TFcard( );
    ~TFcard();

    int init();
    int init(int cs);
};



#ifdef __cplusplus
}
#endif

#endif

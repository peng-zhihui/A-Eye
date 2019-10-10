#ifndef _KPU_WRAPPER_H_
#define _KPU_WRAPPER_H_

#include <bsp.h>
#include <kpu.h>
#include <region_layer.h>
#include <w25qxx.h>
#include "flash-manager.h"
#include "screen.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define CLASS_NUMBER 20
#define ANCHOR_NUM 5

  typedef struct
  {
    char *str;
    uint16_t color;
    uint16_t height;
    uint16_t width;
    uint32_t *ptr;
  } class_lable_t;

  class KPU
  {
  private:
    kpu_model_context_t task;
    region_layer_t detect_rl;

    float g_anchor[ANCHOR_NUM * 2] = {1.08, 1.19, 3.42, 4.41, 6.63,
                                      11.38, 9.42, 5.11, 16.62, 10.52};

    class_lable_t class_lable[CLASS_NUMBER] = {
        {"aeroplane", GREEN}, {"bicycle", GREEN}, {"bird", GREEN}, {"boat", GREEN}, {"bottle", 0xF81F}, {"bus", GREEN}, {"car", GREEN}, {"cat", GREEN}, {"chair", 0xFD20}, {"cow", GREEN}, {"diningtable", GREEN}, {"dog", GREEN}, {"horse", GREEN}, {"motorbike", GREEN}, {"person", 0xF800}, {"pottedplant", GREEN}, {"sheep", GREEN}, {"sofa", GREEN}, {"train", GREEN}, {"tvmonitor", 0xF9B6}};

    void lable_init(void);

  public:
    KPU(/* args */);
    ~KPU();

    volatile static uint8_t g_ai_done_flag;

    uint8_t model[KMODEL_SIZE];

    static uint8_t g_ai_buf[320 * 240 * 3] __attribute__((aligned(128)));
    static uint32_t lable_string_draw_ram[115 * 16 * 8 / 2];

    int init();
    int run();
  };

#ifdef __cplusplus
}
#endif

#endif

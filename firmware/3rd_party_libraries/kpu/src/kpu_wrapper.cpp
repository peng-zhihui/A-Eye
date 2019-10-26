#include "kpu_wrapper.h"

uint8_t KPU::g_ai_buf[3 * 320 * 240] __attribute__((aligned(128)));
uint32_t KPU::lable_string_draw_ram[115 * 16 * 8 / 2];
volatile uint8_t KPU::g_ai_done_flag;

KPU::KPU(/* args */) {}

KPU::~KPU() {}

void KPU::lable_init(void)
{
  uint8_t index;

  class_lable[0].height = 16;
  class_lable[0].width = 8 * strlen(class_lable[0].str);
  class_lable[0].ptr = lable_string_draw_ram;

  for (index = 1; index < CLASS_NUMBER; index++)
  {
    class_lable[index].height = 16;
    class_lable[index].width = 8 * strlen(class_lable[index].str);
    class_lable[index].ptr =
        class_lable[index - 1].ptr +
        class_lable[index - 1].height * class_lable[index - 1].width / 2;
  }
}

void drawboxes(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2,
               uint32_t clazz, float prob)
{
  if (x1 >= 320)
    x1 = 319;
  if (x2 >= 320)
    x2 = 319;
  if (y1 >= 240)
    y1 = 239;
  if (y2 >= 240)
    y2 = 239;

  // lcd.draw_rectangle(x1 / 2, y1 / 2, x2 / 2, y2 / 2, 2,
  //                    class_lable[clazz].color);
}

int KPU::init()
{
  lable_init();

  /* init kpu */
  if (kpu_load_kmodel(&task, model) != 0)
  {
    printf("\nmodel init error\n");
    while (1)
      ;
  }

  detect_rl.anchor_number = ANCHOR_NUM;
  detect_rl.anchor = g_anchor;
  detect_rl.threshold = 0.7;
  detect_rl.nms_value = 0.3;
  region_layer_init(&detect_rl, 10, 7, 125, 320, 240);

  return 0;
}

void ai_done(void *ctx) { KPU::g_ai_done_flag = 1; }

int KPU::run()
{
  kpu_run_kmodel(&task, g_ai_buf, DMAC_CHANNEL5, ai_done, NULL);
  while (!g_ai_done_flag)
    ;
  g_ai_done_flag = 0;

  float *output;
  size_t output_size;
  kpu_get_output(&task, 0, (uint8_t **)(&output), &output_size);
  detect_rl.input = output;

  // for (int i = 0; i < output_size / 4; i++)
  // {
  //   printf("%.3f ", *(output + i));
  // }
  // printf("\n");

  /* start region layer */
  region_layer_run(&detect_rl, NULL);

  region_layer_draw_boxes(&detect_rl, drawboxes);

  return 0;
}

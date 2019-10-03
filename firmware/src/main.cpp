#include "a_eye_bsp.h"

int main(void)
{
  /* init */
  system_init();

  /* enable core1 */
  start_core1(main_core1, (void *)("Hello, world! From core1"));

  /* loop start */
  while (true)
  {
    if (camera.grab_frame_done())
    {
      camera.switch_gram();

      /* process frame */
      // camera.save_picture();

      /* display frame */
      lcd.draw_picture_resized(0, 0, 160, 120, camera.get_frame_ptr());
    }
  }

  return 0;
}

int main_core1(void *ctx)
{
  while (true)
  {
    printf("%s\n", (char *)ctx);

    delay(1000);
  }

  return 0;
}

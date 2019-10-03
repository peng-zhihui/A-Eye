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
  double cnt = 0.01;
  int flag = 0;

  while (true)
  {
    flag ? (cnt -= 0.01) : (cnt += 0.01);
    if (cnt > 1.0)
    {
      cnt = 1.0;
      flag = 1;
    }
    else if (cnt < 0.0)
    {
      cnt = 0.0;
      flag = 0;
    }

    set_led(cnt);

    delay(30);
  }

  return 0;
}

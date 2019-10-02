#include "a_eye_bsp.h"

int foo(void *ctx);

int main(void)
{
  /* init */
  system_init();

  run_on_core1(foo, (void *)("core1"));
  foo((void *)("core0"));

  /* loop start */
  while (true)
  {
    if (camera.grab_frame_done())
    {
      camera.switch_gram();

      /* process frame */
      //camera.save_picture();

      /* display frame */
      lcd.draw_picture_half(0, 0, 320, 240, camera.get_frame_ptr());
    }
  }

  return 0;
}

int foo(void *ctx)
{
  uint64_t core_id = current_coreid();

  if (core_id == 0)
  {
  }
  else
  {
  }

  printf("Hello, world! From %s\n", (char *)ctx);

  return 0;
}

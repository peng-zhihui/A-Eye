#include "a_eye_bsp.h"

int main(void) {
  /* init */
  system_init();

  /* loop start */
  while (true) {
    if (camera.grab_frame_done()) {
      /* process frame */
      nn.run();

      /* switch camera buffer */
      camera.switch_gram();

      /* display frame */
      lcd.draw_picture_resized(40, 0, 160, 120, camera.get_frame_ptr());
      // camera.save_picture();
    }
  }

  return 0;
}

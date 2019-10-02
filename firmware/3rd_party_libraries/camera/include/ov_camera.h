#ifndef _OV_CAMERA_H_
#define _OV_CAMERA_H_

#include <bsp.h>
#include <camera.h>
#include <dvp.h>
#include <plic.h>
#include <rgb2bmp.h>
#include <fpioa.h>
#include <ff.h>

#ifdef __cplusplus
extern "C"
{
#endif

    int on_irq_dvp(void *ctx);

    class Camera
    {
    private:
        static uint32_t g_lcd_gram0[38400] __attribute__((aligned(64)));
        static uint32_t g_lcd_gram1[38400] __attribute__((aligned(64)));

    public:
        Camera();
        ~Camera();

        static volatile uint8_t g_ram_mux;
        static volatile uint8_t g_dvp_finish_flag;

        void init();
        void save_picture();

        uint32_t *get_frame_ptr();
        void switch_gram();
        bool grab_frame_done();

        friend int on_irq_dvp(void *ctx);
    };

#ifdef __cplusplus
}
#endif

#endif

#ifndef _LCD_H_
#define _LCD_H_

#include "spi_tft.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "font.h"
#include "sleep.h"
#include "stdlib.h"
#include "math.h"
#include <bsp.h>
#include <pwm.h>

#define BLACK 0x0000
#define NAVY 0x000F
#define DARKGREEN 0x03E0
#define DARKCYAN 0x03EF
#define MAROON 0x7800
#define PURPLE 0x780F
#define OLIVE 0x7BE0
#define LIGHTGREY 0xC618
#define DARKGREY 0x7BEF
#define BLUE 0x001F
#define GREEN 0x07E0
#define CYAN 0x07FF
#define RED 0xF800
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF
#define ORANGE 0xFD20
#define GREENYELLOW 0xAFE5
#define PINK 0xF81F
#define USER_COLOR 0xAA55

#define SWAP_16(x) ((x >> 8 & 0xff) | (x << 8))

#ifdef __cplusplus
extern "C"
{
#endif

    // Add more screens here, and provide init functions in screen.cpp
    typedef enum ScreenType
    {
        ST7735_160x80_0d96,
        ST7789_320x240_2d4,
        ST7789_240x135_1d14
    } ScreenType;

    typedef enum _lcd_dir
    {
        DIR_XY_RLUD = 0x00,
        DIR_YX_RLUD = 0x20,
        DIR_XY_LRUD = 0x40,
        DIR_YX_LRUD = 0x60,
        DIR_XY_RLDU = 0x80,
        DIR_YX_RLDU = 0xA0,
        DIR_XY_LRDU = 0xC0,
        DIR_YX_LRDU = 0xE0,
        DIR_XY_MASK = 0x20,
        DIR_MASK = 0xE0,
    } lcd_dir_t;

    typedef struct _lcd_ctl
    {
        uint8_t mode;
        uint8_t dir;
        uint16_t width;
        uint16_t height;
        uint16_t start_offset_w0;
        uint16_t start_offset_h0;
        uint16_t start_offset_w;
        uint16_t start_offset_h;
    } lcd_ctl_t;

    uint16_t rgb_24_to_565(uint8_t r, uint8_t g, uint8_t b);

    class Screen
    {
    private:
        lcd_ctl_t lcd_ctl; // lcd params
        uint16_t g_lcd_w;  // lcd width
        uint16_t g_lcd_h;  // lcd height
        bool g_lcd_init;   // lcd init flag

        static uint16_t resized_frame[320 * 240];

        void set_area(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
        void set_direction(lcd_dir_t dir);
        void enable_polling(void);
        void enable_interrupt(void);

    public:
        Screen();
        ~Screen();

        int init(ScreenType screenType);
        int init(ScreenType screenType, int rst, int dc);

        int get_width();
        int get_height();

        void clear(uint16_t color);

        void draw_point(uint16_t x, uint16_t y, uint16_t color);
        void draw_char(uint16_t x, uint16_t y, char c, uint16_t color);
        void draw_string(uint16_t x, uint16_t y, char *str, uint16_t color);

        void fill_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
        void draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t width, uint16_t color);

        void draw_picture(uint16_t x1, uint16_t y1, uint16_t width, uint16_t height, uint32_t *ptr);
        void draw_pic_roi(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t rx, uint16_t ry, uint16_t rw, uint16_t rh, uint32_t *ptr);
        void draw_picture_resized(uint16_t x1, uint16_t y1, uint16_t width, uint16_t height, uint32_t *ptr);

        friend int resize(void *ctx);

        void set_backlight(float percent);

        // For ST7789_240x135_1d14
        friend int init_st7789_240x135(Screen *context);
    };

#ifdef __cplusplus
}
#endif

#endif

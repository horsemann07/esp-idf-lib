
#ifndef _SSD1306_H_
#define _SSD1306_H_

#include <i2cdev.h>

typedef enum {
    COLOR_WHITE = 1, //
    COLOR_BLACK = 2,
    COLOR_INVERSE = 3
} ssd1306_color_t;

typedef enum {
    DISP_w128xh32 = 1, //
    DISP_w128xh64 = 2,
    DISP_w96xh16 = 3
} disp_type_t;

typedef enum {
    SCROLL_RIGHT = 1, //
    SCROLL_LEFT = 2,
    SCROLL_DOWN = 3,
    SCROLL_UP = 4,
    SCROLL_STOP = 5
} ssd1306_scroll_dir_t;

typedef enum {
    SCROLL_SPEED_0 = 0x03, // slowest
    SCROLL_SPEED_1 = 0x02,
    SCROLL_SPEED_2 = 0x01,
    SCROLL_SPEED_3 = 0x06,
    SCROLL_SPEED_4 = 0x00,
    SCROLL_SPEED_5 = 0x05,
    SCROLL_SPEED_6 = 0x04,
    SCROLL_SPEED_7 = 0x07 // fastest
} ssd1306_scroll_speed_t;

typedef enum {
    LANDSCAPE = 0,      //
    LANDSCAPE_WIDE = 1, //
    PORTRAIT = 2,
    PORTRAIT_TALL = 3 //
} ssd1306_orentation_t;

typedef enum {
    PAGE1 = 0, //
    PAGE2,
    PAGE3,
    PAGE4,
    PAGE5,
    PAGE6,
    PAGE7
} page_area_t;

typedef struct
{
    bool invert;
    int16_t rstpin;
    int16_t width;
    int16_t height;
    int16_t spages; // supported pages based on height and width
    // int16_t dc;
    int16_t row;
    int16_t column;
    uint16_t dispbuflen;
    uint8_t *dispbuf;
    i2c_dev_t i2c_dev;
} ssd1306_t;

typedef struct
{
    ssd1306_t ssd1306;
    int16_t vwidth;  // volatile width
    int16_t vheight; // volatile height
    int16_t cursor_x;
    int16_t cursor_y;
    uint16_t textcolor;
    uint16_t textbgcolor;
    uint8_t textsize;
    ssd1306_orentation_t orentation;
    uint8_t wrap;
} __attribute__((packed)) ssd1306_gfx_t;

// see data sheet page 25 for Graphic Display Data RAM organization
// 8 spages, each page a row of DISPLAYWIDTH bytes
// start address of of row: y/8*DISPLAYWIDTH
// x pos in row: == x
inline uint8_t *GDDRAM_ADDRESS(ssd1306_t *ssd1306, int16_t x, int16_t y)
{
    return (((ssd1306->dispbuf) + ((y) / 8) * (ssd1306->width) + (x)));
}

// lower 3 bit of y determine vertical pixel position (pos 0...7) in GDDRAM byte
// (y&0x07) == position of pixel on row (page). LSB is top, MSB bottom
inline int16_t GDDRAM_PIXMASK(ssd1306_t *ssd1306, int16_t y) { return (1 << ((y)&ssd1306->spages)); }

esp_err_t ssd1306_gfx_rotation_adjust(ssd1306_gfx_t *ssd1306_gfx, int16_t *px, int16_t *py);
esp_err_t ssd1306_gfx_draw_line(ssd1306_gfx_t *ssd1306_gfx, int16_t x0, int16_t y0, int16_t x1, int16_t y1,
    ssd1306_color_t color);
esp_err_t ssd1306_stop_scroll(ssd1306_t *ssd1306);
esp_err_t ssd1306_scroll(ssd1306_t *ssd1306, page_area_t start, page_area_t end, ssd1306_scroll_dir_t dir,
    ssd1306_scroll_speed_t speed);
esp_err_t ssd1306_gfx_set_cursor(ssd1306_gfx_t *ssd1306_gfx, int16_t x, int16_t y);
esp_err_t ssd1306_gfx_set_textsize(ssd1306_gfx_t *ssd1306_gfx, uint8_t size);
esp_err_t ssd1306_gfx_set_textcolor(ssd1306_gfx_t *ssd1306_gfx, uint16_t color);
esp_err_t ssd1306_gfx_set_textbg(ssd1306_gfx_t *ssd1306_gfx, uint16_t color);
esp_err_t ssd1306_gfx_set_textwrap(ssd1306_gfx_t *ssd1306_gfx, uint8_t w);

esp_err_t ssd1306_gfx_set_orentation(ssd1306_gfx_t *ssd1306_gfx, uint8_t orentation);
esp_err_t ssd1306_gfx_init_desc(ssd1306_gfx_t *ssd1306_gfx, uint32_t sda, uint32_t scl, int port, uint32_t reset_pin,
    bool flip, disp_type_t display, uint8_t vccstate);
esp_err_t ssd1306_gfx_deinit_desc(ssd1306_gfx_t *ssd1306_gfx);
esp_err_t ssd1306_set_pixels(ssd1306_t *ssd1306, int16_t x, int16_t y, ssd1306_color_t color);
esp_err_t ssd1306_screen_off(ssd1306_t *ssd1306);
esp_err_t ssd1306_screen_on(ssd1306_t *ssd1306);
esp_err_t ssd1306_set_cursor(ssd1306_t *ssd1306, uint8_t lineNumber, uint8_t cursorPosition);
esp_err_t ssd1306_set_brightness(ssd1306_t *ssd1306, uint8_t percent);
esp_err_t ssd1306_clear_display(ssd1306_t *ssd1306);
esp_err_t ssd1306_set_inverse(ssd1306_t *ssd1306, bool flip);
esp_err_t ssd1306_set_dimension(ssd1306_t *ssd1306, disp_type_t display);
esp_err_t ssd1306_init_desc(ssd1306_t *ssd1306, uint32_t sda, uint32_t scl, int port, uint32_t reset_pin, bool flip,
    disp_type_t display, uint8_t vccstate);
esp_err_t ssd1306_deinit_desc(ssd1306_t *ssd1306);

#endif /* _SSD1306_H_ */
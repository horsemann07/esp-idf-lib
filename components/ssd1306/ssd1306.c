
#include "ssd1306.h"
#include <esp_err.h>
#include <esp_log.h>

#include <i2cdev.h>
#include <string.h>

#define TAG (__FILENAME__)

#define LOG_LOCAL_LEVEL (ESP_LOG_DEBUG)

/// fit into the SSD1306_ naming scheme
// #define SSD1306_BLACK   (0) ///< Draw 'off' pixels
// #define SSD1306_WHITE   (1) ///< Draw 'on' pixels
// #define SSD1306_INVERSE (2) ///< Invert pixels

#define SSD1306_MEMORYMODE          (0x20) ///< See datasheet
#define SSD1306_COLUMNADDR          (0x21) ///< See datasheet
#define SSD1306_PAGEADDR            (0x22) ///< See datasheet
#define SSD1306_SETCONTRAST         (0x81) ///< See datasheet
#define SSD1306_CHARGEPUMP          (0x8D) ///< See datasheet
#define SSD1306_SEGREMAP            (0xA0) ///< See datasheet
#define SSD1306_DISPLAYALLON_RESUME (0xA4) ///< See datasheet
#define SSD1306_DISPLAYALLON        (0xA5) ///< Not currently used
#define SSD1306_NORMALDISPLAY       (0xA6) ///< See datasheet
#define SSD1306_INVERTDISPLAY       (0xA7) ///< See datasheet
#define SSD1306_SETMULTIPLEX        (0xA8) ///< See datasheet
#define SSD1306_DISPLAYOFF          (0xAE) ///< See datasheet
#define SSD1306_DISPLAYON           (0xAF) ///< See datasheet
#define SSD1306_COMSCANINC          (0xC0) ///< Not currently used
#define SSD1306_COMSCANDEC          (0xC8) ///< See datasheet
#define SSD1306_SETDISPLAYOFFSET    (0xD3) ///< See datasheet
#define SSD1306_SETDISPLAYCLOCKDIV  (0xD5) ///< See datasheet
#define SSD1306_SETPRECHARGE        (0xD9) ///< See datasheet
#define SSD1306_SETCOMPINS          (0xDA) ///< See datasheet
#define SSD1306_SETVCOMDETECT       (0xDB) ///< See datasheet

#define SSD1306_SETLOWCOLUMN  (0x00) ///< Not currently used
#define SSD1306_SETHIGHCOLUMN (0x10) ///< Not currently used
#define SSD1306_SETSTARTLINE  (0x40) ///< See datasheet

// power
#define SSD1306_EXTERNALVCC  (0x01) ///< External display voltage source
#define SSD1306_SWITCHCAPVCC (0x02) ///< Gen. display voltage from 3.3V

// Scroll
#define SSD1306_RIGHT_HORIZONTAL_SCROLL              (0x26) ///< Init rt scroll
#define SSD1306_LEFT_HORIZONTAL_SCROLL               (0x27) ///< Init left scroll
#define SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL (0x29) ///< Init diag scroll
#define SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL  (0x2A) ///< Init diag scroll
#define SSD1306_DEACTIVATE_SCROLL                    (0x2E) ///< Stop scroll
#define SSD1306_ACTIVATE_SCROLL                      (0x2F) ///< Start scroll
#define SSD1306_SET_VERTICAL_SCROLL_AREA             (0xA3) ///< Set scroll range

// command and data control
#define SSD1306_CONTROL_BYTE_CMD_SINGLE  (0x80)
#define SSD1306_CONTROL_BYTE_CMD_STREAM  (0x00)
#define SSD1306_CONTROL_BYTE_DATA_SINGLE (0xC0)
#define SSD1306_CONTROL_BYTE_DATA_STREAM (0x40)

#define I2C_FREQ_HZ (100000)

#define ARRAY_SIZE(x)     ((size_t)(sizeof(x) / sizeof(x[0])))
#define PERCENTAGE(input) ((uint8_t)((input - min) * 100) / (max - min))
#define CONTRAST(x)       ((uint8_t)((x * (255 - 0) / 100) + 0))

/**
 * Macro which can be used to check parameters.
 * Prints the error code, error location, and the failed statement to serial output.
 *
 */
#define ESP_PARAM_CHECK(con)                                                                                           \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(con))                                                                                                    \
        {                                                                                                              \
            ESP_LOGE(TAG, "[%s, %d]: <ESP_ERR_INVALID_ARG> !(%s)", __func__, __LINE__, #con);                          \
            return ESP_ERR_INVALID_ARG;                                                                                \
        }                                                                                                              \
    }                                                                                                                  \
    while (0)

/*
 * Macro which can be used to check the error code,
 * and terminate the program in case the code is not ESP_OK.
 */
#define ESP_ERROR_RETURN(ret)                                                                                          \
    do                                                                                                                 \
    {                                                                                                                  \
        esp_err_t __ = ret;                                                                                            \
        if (__ != ESP_OK)                                                                                              \
        {                                                                                                              \
            return __;                                                                                                 \
        }                                                                                                              \
    }                                                                                                                  \
    while (0)

/**
 * Macro which can be used to check the error code,
 * and terminate the program in case the code is not ESP_OK.
 * In debug mode, it prints the error code, error location to serial output.
 */
#define ESP_ERROR_CDEBUG(ret)                                                                                          \
    if (ret != ESP_OK)                                                                                                 \
    {                                                                                                                  \
        ESP_LOGD(TAG, "[%s, %d] <%s> ", __func__, __LINE__, esp_err_to_name(ret));                                     \
        return ret;                                                                                                    \
    }

/**
 * Macro which can be used to check the error code,
 * and terminate the program in case the code is not ESP_OK.
 * In debug mode, it prints the error code, error location to serial output.
 */
#define ESP_ERR_ON_INIT_FAIL(ret)                                                                                      \
    if (ret != ESP_OK)                                                                                                 \
    {                                                                                                                  \
        ESP_LOGD(TAG, "[%s, %d] <%s> ", __func__, __LINE__, esp_err_to_name(ret));                                     \
        goto END;                                                                                                      \
    }

///< No-temp-var swap operation
#define SWAP(a, b)      (((a) ^= (b)), ((b) ^= (a)), ((a) ^= (b)))
#define BIT_SET(a, b)   ((a) |= (1U << (b)))
#define BIT_CLEAR(a, b) ((a) &= ~(1U << (b)))

// see data sheet page 25 for Graphic Display Data RAM organization
// 8 spages, each page a row of DISPLAYWIDTH bytes
// start address of of row: y/8*DISPLAYWIDTH
// x pos in row: == x
// #define GDDRAM_ADDRESS(dispbuf, width, X, Y) ((dispbuf) + ((Y) / 8) * (ssd1306->width) + (X))

// // lower 3 bit of y determine vertical pixel position (pos 0...7) in GDDRAM byte
// // (y&0x07) == position of pixel on row (page). LSB is top, MSB bottom
// #define GDDRAM_PIXMASK(spages, Y) (1 << ((Y)&spages))

#define PIXEL_ON(dispbuf, X, Y)     (*GDDRAM_ADDRESS(ssd1306, x, y) |= GDDRAM_PIXMASK(ssd1306, y))
#define PIXEL_OFF(dispbuf, X, Y)    (*GDDRAM_ADDRESS(ssd1306, x, y) &= ~GDDRAM_PIXMASK(ssd1306, y))
#define PIXEL_TOGGLE(dispbuf, X, Y) (*GDDRAM_ADDRESS(ssd1306, x, y) ^= GDDRAM_PIXMASK(ssd1306, y))

esp_err_t ssd1306_write_commands(const i2c_dev_t *i2c_dev, const uint8_t *commnadlist, size_t length)
{
    ESP_ERROR_CDEBUG(i2c_dev_write_reg(i2c_dev, 0, (const void *)SSD1306_CONTROL_BYTE_CMD_STREAM, 1));
    return (i2c_dev_write(i2c_dev, NULL, 0, (const void *)&commnadlist, length));
}

esp_err_t ssd1306_write_command(const i2c_dev_t *i2c_dev, uint8_t commnad)
{
    ESP_ERROR_CDEBUG(i2c_dev_write_reg(i2c_dev, 0, (const void *)SSD1306_CONTROL_BYTE_CMD_SINGLE, 1));
    return (i2c_dev_write_reg(i2c_dev, 0, (const void *)&commnad, 1));
}

esp_err_t ssd1306_write_data_buffer(const i2c_dev_t *i2c_dev, const uint8_t *data, size_t length)
{
    ESP_ERROR_CDEBUG(i2c_dev_write_reg(i2c_dev, 0, (const void *)SSD1306_CONTROL_BYTE_DATA_STREAM, 1));
    return (i2c_dev_write(i2c_dev, NULL, 0, (const void *)&data, length));
}

esp_err_t ssd1306_write_data(const i2c_dev_t *i2c_dev, uint8_t data)
{
    ESP_ERROR_CDEBUG(i2c_dev_write_reg(i2c_dev, 0, (const void *)SSD1306_CONTROL_BYTE_DATA_SINGLE, 1));
    return (i2c_dev_write_reg(i2c_dev, 0, (const void *)&data, 1));
}

//////////////////////////// GFX LIBRARY //////////////////////////////////////

static esp_err_t ssd1306_validate_cord(ssd1306_gfx_t *ssd1306_gfx, int16_t *x0, int16_t *y0, int16_t *x1, int16_t *y1)
{
    if (!x0 || !y0 || !x1 || !y1 || !ssd1306_gfx)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // bounds check
    if (ssd1306_gfx->orentation == PORTRAIT || ssd1306_gfx->orentation == PORTRAIT_TALL)
    {
        if (*x0 < 0 || *x0 >= ssd1306_gfx->ssd1306.height || *x1 < 0 || *x1 >= ssd1306_gfx->ssd1306.height)
        {
            return ESP_ERR_INVALID_ARG;
        }

        if (*y0 < 0 || *y0 >= ssd1306_gfx->ssd1306.width || *y1 < 0 || *y1 >= ssd1306_gfx->ssd1306.width)
        {
            return ESP_ERR_INVALID_ARG;
        }
    }
    // rotate == LANDSCAPE || rotate == LANDSCAPE_WIDE
    else
    {
        if (*y0 < 0 || *y0 >= ssd1306_gfx->ssd1306.height || *y1 < 0 || *y1 >= ssd1306_gfx->ssd1306.height)
            return ESP_FAIL;
        if (*x0 < 0 || *x0 >= ssd1306_gfx->ssd1306.width || *x1 < 0 || *x1 >= ssd1306_gfx->ssd1306.width)
            return ESP_FAIL;
    }

    ESP_ERROR_CDEBUG(ssd1306_gfx_rotation_adjust(ssd1306_gfx, x0, y0));
    ESP_ERROR_CDEBUG(ssd1306_gfx_rotation_adjust(ssd1306_gfx, x1, y1));

    // ensure coords are from left to right and top to bottom

    if ((*x0 == *x1 && *y1 < *y0) || (*y0 == *y1 && *x1 < *x0))
    {
        // swap as needed
        SWAP(*x0, *x1);
        SWAP(*y0, *y1);
    }

    return ESP_OK;
}

esp_err_t ssd1306_gfx_rotation_adjust(ssd1306_gfx_t *ssd1306_gfx, int16_t *px, int16_t *py)
{
    if (!px || !py || !ssd1306_gfx)
    {
        return ESP_ERR_INVALID_ARG;
    }

    switch (ssd1306_gfx->orentation)
    {
        case PORTRAIT:
            *px = (int16_t)(*py);
            *py = (ssd1306_gfx->vwidth - (int16_t)(*py) - 1);
            break;

        case LANDSCAPE_WIDE:
            *px = (ssd1306_gfx->vwidth - (int16_t)(*px) - 1);
            *py = (ssd1306_gfx->vheight - (int16_t)(*py) - 1);
            break;

        case LANDSCAPE:
            *py = (ssd1306_gfx->vheight - (int16_t)(*px) - 1);
            *px = (int16_t)(*py);
            break;
        default:
            break;
    }
    return ESP_OK;
}

esp_err_t ssd1306_gfx_draw_line(ssd1306_gfx_t *ssd1306_gfx, int16_t x0, int16_t y0, int16_t x1, int16_t y1,
    ssd1306_color_t color)
{
    ESP_PARAM_CHECK(ssd1306_gfx);
    ESP_ERROR_CDEBUG(ssd1306_validate_cord(ssd1306_gfx, &x0, &y0, &x1, &y1));

    esp_err_t ret = ESP_OK;
    if (x0 == x1)
    {
        // vertical
        uint8_t *pstart = GDDRAM_ADDRESS(&ssd1306_gfx->ssd1306, x0, y0);
        uint8_t *pend = GDDRAM_ADDRESS(&ssd1306_gfx->ssd1306, x1, y1);
        uint8_t *ptr = pstart;

        while (ptr <= pend)
        {
            uint8_t mask = 0;
            if (ptr == pstart)
            {
                // top
                uint8_t lbit = y0 % ssd1306_gfx->ssd1306.spages;
                // bottom (line can be very short, all inside this one byte)
                uint8_t ubit = lbit + y1 - y0;
                if (ubit >= ssd1306_gfx->ssd1306.spages)
                {
                    ubit = ssd1306_gfx->ssd1306.spages;
                }

                mask = ((1 << (ubit - lbit + 1)) - 1) << lbit;
            }
            else if (ptr == pend)
            {
                // top is always bit 0, that makes it easy
                // bottom
                mask = (1 << (y1 % ssd1306_gfx->ssd1306.spages)) - 1;
            }

            if (ptr == pstart || ptr == pend)
            {
                switch (color)
                {
                    case COLOR_WHITE:
                        *ptr |= mask;
                        break;
                    case COLOR_BLACK:
                        *ptr &= ~mask;
                        break;
                    case COLOR_INVERSE:
                        *ptr ^= mask;
                        break;
                    default:
                        ret = ESP_FAIL;
                        break;
                };
            }
            else
            {
                switch (color)
                {
                    case COLOR_WHITE:
                        *ptr = 0xff;
                        break;
                    case COLOR_BLACK:
                        *ptr = 0x00;
                        break;
                    case COLOR_INVERSE:
                        *ptr ^= 0xff;
                        break;
                    default:
                        ret = ESP_FAIL;
                        break;
                };
            }

            ptr += ssd1306_gfx->ssd1306.width;
        }
    }
    else
    {
        // horizontal
        uint8_t *pstart = GDDRAM_ADDRESS(&ssd1306_gfx->ssd1306, x0, y0);
        uint8_t *pend = pstart + x1 - x0;
        uint8_t pixmask = GDDRAM_PIXMASK(&ssd1306_gfx->ssd1306, y0);

        uint8_t *ptr = pstart;
        while (ptr <= pend)
        {
            switch (color)
            {
                case COLOR_WHITE:
                    *ptr |= pixmask;
                    break;
                case COLOR_BLACK:
                    *ptr &= ~pixmask;
                    break;
                case COLOR_INVERSE:
                    *ptr ^= pixmask;
                    break;
                default:
                    ret = ESP_FAIL;
                    break;
            };
            ptr++;
        }
    }
    return ret;
}

esp_err_t ssd1306_stop_scroll(ssd1306_t *ssd1306)
{
    ESP_PARAM_CHECK(ssd1306);
    return (ssd1306_write_command(&(ssd1306->i2c_dev), SSD1306_DEACTIVATE_SCROLL));
}

esp_err_t ssd1306_scroll(ssd1306_t *ssd1306, page_area_t start, page_area_t end, ssd1306_scroll_dir_t dir,
    ssd1306_scroll_speed_t speed)
{
    ESP_PARAM_CHECK(ssd1306);

    // set boundary start and end pg based on support pages
    start = ((start < 0) ? 0 : start);
    end = ((end >= ssd1306->spages) ? (ssd1306->spages - 1) : end);
    if (start > end)
    {
        return ESP_ERR_INVALID_ARG;
    }

    const uint8_t cmdbuf[] = {
        dir,                    // left right up down
        0x00,                   // dummy byte
        start,                  // start page
        speed,                  // scroll step interval in terms of frame frequency
        end,                    // end page
        0x00,                   // dummy byte
        0xFF,                   // dummy byte
        SSD1306_ACTIVATE_SCROLL // 0x2F
    };
    return (ssd1306_write_commands(&(ssd1306->i2c_dev), (const uint8_t *)cmdbuf, ARRAY_SIZE(cmdbuf)));
}

esp_err_t ssd1306_gfx_set_cursor(ssd1306_gfx_t *ssd1306_gfx, int16_t x, int16_t y)
{
    ESP_PARAM_CHECK(ssd1306_gfx);
    ssd1306_gfx->cursor_x = x;
    ssd1306_gfx->cursor_y = y;
    return ESP_OK;
}

esp_err_t ssd1306_gfx_set_textsize(ssd1306_gfx_t *ssd1306_gfx, uint8_t size)
{
    ESP_PARAM_CHECK(ssd1306_gfx);
    ssd1306_gfx->textsize = (size > 0) ? size : 1;
    return ESP_OK;
}

esp_err_t ssd1306_gfx_set_textcolor(ssd1306_gfx_t *ssd1306_gfx, uint16_t color)
{
    ESP_PARAM_CHECK(ssd1306_gfx);
    // For 'transparent' background, we'll set the bg
    // to the same as fg instead of using a flag
    ssd1306_gfx->textcolor = color;
    ssd1306_gfx->textbgcolor = color;
    return ESP_OK;
}

esp_err_t ssd1306_gfx_set_textbg(ssd1306_gfx_t *ssd1306_gfx, uint16_t color)
{
    ESP_PARAM_CHECK(ssd1306_gfx);
    ssd1306_gfx->textbgcolor = color;
    return ESP_OK;
}

esp_err_t ssd1306_gfx_set_textwrap(ssd1306_gfx_t *ssd1306_gfx, uint8_t w)
{
    ESP_PARAM_CHECK(ssd1306_gfx);
    ssd1306_gfx->wrap = w;
    return ESP_OK;
}

esp_err_t ssd1306_gfx_set_orentation(ssd1306_gfx_t *ssd1306_gfx, uint8_t orentation)
{
    ESP_PARAM_CHECK(ssd1306_gfx);
    ssd1306_gfx->orentation = (orentation & 3);
    switch (ssd1306_gfx->orentation)
    {
        case PORTRAIT_TALL:
        case PORTRAIT:
            ssd1306_gfx->vwidth = ssd1306_gfx->ssd1306.height;
            ssd1306_gfx->vheight = ssd1306_gfx->ssd1306.width;
            break;

        case LANDSCAPE_WIDE:
        case LANDSCAPE:
            ssd1306_gfx->vwidth = ssd1306_gfx->ssd1306.width;
            ssd1306_gfx->vheight = ssd1306_gfx->ssd1306.height;
            break;
        default:
            break;
    }
    return ESP_OK;
}

esp_err_t ssd1306_gfx_init_desc(ssd1306_gfx_t *ssd1306_gfx, uint32_t sda, uint32_t scl, int port, uint32_t reset_pin,
    bool invert, disp_type_t display, uint8_t vccstate)
{
    ESP_PARAM_CHECK(ssd1306_gfx);

    ESP_ERROR_CDEBUG(ssd1306_init_desc(&(ssd1306_gfx->ssd1306), sda, scl, port, reset_pin, invert, display, vccstate));
    ssd1306_gfx->cursor_x = 0;
    ssd1306_gfx->cursor_y = 0;
    ssd1306_gfx->orentation = LANDSCAPE;
    ssd1306_gfx->textbgcolor = 0xFFFF;
    ssd1306_gfx->textcolor = 0xFFFF;
    ssd1306_gfx->textsize = 1;
    ssd1306_gfx->vheight = ssd1306_gfx->ssd1306.height;
    ssd1306_gfx->vwidth = ssd1306_gfx->ssd1306.width;
    ssd1306_gfx->wrap = 1;
    return ESP_OK;
}

esp_err_t ssd1306_gfx_deinit_desc(ssd1306_gfx_t *ssd1306_gfx)
{
    ESP_PARAM_CHECK(ssd1306_gfx);

    ESP_ERROR_CDEBUG(ssd1306_deinit_desc(&(ssd1306_gfx->ssd1306)));
    ssd1306_gfx->cursor_x = 0;
    ssd1306_gfx->cursor_y = 0;
    ssd1306_gfx->orentation = 0;
    ssd1306_gfx->textbgcolor = 0;
    ssd1306_gfx->textcolor = 0;
    ssd1306_gfx->textsize = 0;
    ssd1306_gfx->vheight = 0;
    ssd1306_gfx->vwidth = 0;
    ssd1306_gfx->wrap = 0;
    return ESP_OK;
}
////////////////////////////////////////////////////////////////////////////////////////

static esp_err_t ssd1306_set_contrast(ssd1306_t *ssd1306, uint8_t contrast)
{
    ESP_ERROR_CDEBUG(ssd1306_write_command(&(ssd1306->i2c_dev), SSD1306_SETCONTRAST));
    ESP_ERROR_CDEBUG(ssd1306_write_command(&(ssd1306->i2c_dev), contrast));
    return ESP_OK;
}

esp_err_t ssd1306_set_pixels(ssd1306_t *ssd1306, int16_t x, int16_t y, ssd1306_color_t color)
{
    esp_err_t ret = ESP_OK;
    if ((x < 0) || (x >= ssd1306->width) || (y < 0) || (y >= ssd1306->height))
    {
        return ESP_ERR_INVALID_ARG;
    }

    switch (color)
    {
        case COLOR_WHITE:
            PIXEL_ON(ssd1306, x, y);
            break;
        case COLOR_BLACK:
            PIXEL_OFF(ssd1306, x, y);
            break;
        case COLOR_INVERSE:
            PIXEL_TOGGLE(ssd1306, x, y);
            break;
        default:
            ESP_LOGE(TAG, "unknown color.");
            ret = ESP_FAIL;
            break;
    }
    return ret;
}

esp_err_t ssd1306_screen_off(ssd1306_t *ssd1306)
{
    ESP_PARAM_CHECK(ssd1306);

    return (ssd1306_write_command(&ssd1306->i2c_dev, SSD1306_DISPLAYOFF));
}

esp_err_t ssd1306_screen_on(ssd1306_t *ssd1306)
{
    ESP_PARAM_CHECK(ssd1306);

    return (ssd1306_write_command(&ssd1306->i2c_dev, SSD1306_DISPLAYON));
}

esp_err_t ssd1306_set_cursor(ssd1306_t *ssd1306, uint8_t lineNumber, uint8_t cursorPosition)
{
    /* Move the Cursor to specified position only if it is in range */
    if (!(lineNumber < ssd1306->spages) && !(cursorPosition < ssd1306->width))
    {
        return ESP_ERR_INVALID_ARG;
    }

    const uint8_t cmd[] = { SSD1306_COLUMNADDR, cursorPosition, (ssd1306->width - 1),
        // set page address
        SSD1306_PAGEADDR, lineNumber, (ssd1306->spages) };
    if ((ssd1306_write_commands(&(ssd1306->i2c_dev), (const uint8_t *)cmd, ARRAY_SIZE(cmd))) == ESP_OK)
    {
        ssd1306->row = lineNumber;
        ssd1306->column = cursorPosition;
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t ssd1306_set_brightness(ssd1306_t *ssd1306, uint8_t percent)
{
    ESP_PARAM_CHECK(ssd1306);

    return (ssd1306_set_contrast(ssd1306, CONTRAST(percent)));
}

esp_err_t ssd1306_clear_display(ssd1306_t *ssd1306)
{
    ESP_PARAM_CHECK(ssd1306);

    const uint8_t clear1[] = { SSD1306_COLUMNADDR, SSD1306_SETLOWCOLUMN, (ssd1306->width - 1) };
    ESP_ERROR_CDEBUG(ssd1306_write_commands(&(ssd1306->i2c_dev), (const uint8_t *)clear1, ARRAY_SIZE(clear1)));

    const uint8_t clear2[] = { SSD1306_PAGEADDR, 0, (ssd1306->spages - 1) };
    ESP_ERROR_CDEBUG(ssd1306_write_commands(&(ssd1306->i2c_dev), (const uint8_t *)clear2, ARRAY_SIZE(clear2)));

    // write all zero to clear display.
    // size_t area = ssd1306->width * ssd1306->height;
    // uint8_t dispZero[area];
    // memset((void *)dispZero, 0, area);
    memset((void *)ssd1306->dispbuf, 0, ssd1306->dispbuflen);

    ESP_ERROR_CDEBUG(ssd1306_write_data_buffer(&(ssd1306->i2c_dev), ssd1306->dispbuf, ssd1306->dispbuflen));

    const uint8_t clear3[] = { SSD1306_COLUMNADDR, SSD1306_SETLOWCOLUMN, (ssd1306->width - 1) };
    ESP_ERROR_CDEBUG(ssd1306_write_commands(&(ssd1306->i2c_dev), (const uint8_t *)clear3, ARRAY_SIZE(clear3)));

    const uint8_t clear4[] = { SSD1306_PAGEADDR, 0, (ssd1306->spages - 1) };
    ESP_ERROR_CDEBUG(ssd1306_write_commands(&(ssd1306->i2c_dev), (const uint8_t *)clear4, ARRAY_SIZE(clear4)));
    return ESP_OK;
}

esp_err_t ssd1306_set_inverse(ssd1306_t *ssd1306, bool invert)
{
    ESP_PARAM_CHECK(ssd1306);
    ESP_ERROR_CDEBUG(
        ssd1306_write_command(&(ssd1306->i2c_dev), (invert == true) ? SSD1306_INVERTDISPLAY : SSD1306_NORMALDISPLAY));
    return ESP_OK;
}

esp_err_t ssd1306_set_dimension(ssd1306_t *ssd1306, disp_type_t display)
{
    ESP_PARAM_CHECK(ssd1306);
    esp_err_t ret = ESP_FAIL;
    if (display == DISP_w128xh32)
    {
        ssd1306->height = 32;
        ssd1306->width = 128;
        ssd1306->spages = 4;
        ret = ESP_OK;
    }
    else if (display == DISP_w128xh64)
    {
        ssd1306->height = 64;
        ssd1306->width = 128;
        ssd1306->spages = 8;
        ret = ESP_OK;
    }
    else if (display == DISP_w96xh16)
    {
        ssd1306->height = 16;
        ssd1306->width = 96;
        ssd1306->spages = 2;
        ret = ESP_OK;
    }
    return ret;
}

esp_err_t ssd1306_init_desc(ssd1306_t *ssd1306, uint32_t sda, uint32_t scl, int port, uint32_t reset_pin, bool invert,
    disp_type_t display, uint8_t vccstate)
{
    ESP_PARAM_CHECK(ssd1306);
    esp_err_t ret = ESP_FAIL;

    ESP_ERROR_CDEBUG(ssd1306_set_dimension(ssd1306, display));
    ssd1306->invert = invert;
    ssd1306->row = 0;
    ssd1306->column = 0;
    ssd1306->dispbuflen = ((ssd1306->height * ssd1306->width) / ssd1306->spages + 1);
    ssd1306->dispbuf = calloc(ssd1306->dispbuflen, sizeof(uint8_t));
    if (!ssd1306->dispbuf)
    {
        return ESP_ERR_NO_MEM;
    }
    // If I2C address is unspecified, use default
    // (0x3C for 32-pixel-tall displays, 0x3D for all others).
    ssd1306->i2c_dev.addr = (uint8_t)((ssd1306->height == 32) ? 0x3C : 0x3D);
    ssd1306->i2c_dev.port = port;
    ssd1306->rstpin = reset_pin;
    ssd1306->i2c_dev.cfg.scl_io_num = scl;
    ssd1306->i2c_dev.cfg.sda_io_num = sda;
#if HELPER_TARGET_IS_ESP32
    ssd1306->i2c_dev.cfg.master.clk_speed = I2C_FREQ_HZ;
#endif
    ESP_LOGI(TAG, "i2c_dev: addr 0x%x: scl: %d, sda: %d, port %d", ssd1306->i2c_dev.addr,
        ssd1306->i2c_dev.cfg.scl_io_num, ssd1306->i2c_dev.cfg.sda_io_num, ssd1306->i2c_dev.port);
    // init i2c
    ret = i2c_dev_probe((const i2c_dev_t *)&ssd1306->i2c_dev, I2C_DEV_READ);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "[%s, %d] device not available <%s>", __func__, __LINE__, esp_err_to_name(ret));
        // return ret;
    }

    ret = i2c_dev_create_mutex(&(ssd1306->i2c_dev));
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "[%s, %d] failed to create mutex for i2c dev. <%s>", __func__, __LINE__, esp_err_to_name(ret));
        goto END;
    }

    // Init sequence
    const uint8_t init1[] = { SSD1306_DISPLAYOFF, // 0xAE
        SSD1306_SETDISPLAYCLOCKDIV,               // 0xD5
        0x80,                                     // the suggested ratio 0x80
        SSD1306_SETMULTIPLEX };                   // 0xA8

    ESP_ERR_ON_INIT_FAIL(ssd1306_write_commands(&(ssd1306->i2c_dev), (const uint8_t *)init1, ARRAY_SIZE(init1)));

    const uint8_t init2[] = { SSD1306_SETDISPLAYOFFSET, // 0xD3
        0x0,                                            // no offset
        SSD1306_SETSTARTLINE | 0x0,                     // line #0
        SSD1306_CHARGEPUMP };                           // 0x8D
    ESP_ERR_ON_INIT_FAIL(ssd1306_write_commands(&(ssd1306->i2c_dev), (const uint8_t *)init2, ARRAY_SIZE(init2)));

    ESP_ERR_ON_INIT_FAIL(ssd1306_write_command(&(ssd1306->i2c_dev), (vccstate == SSD1306_EXTERNALVCC) ? 0x10 : 0x14));
    // ssd1306->dc = ((vccstate == SSD1306_EXTERNALVCC) ? 1 : 0);
    const uint8_t init3[] = { SSD1306_MEMORYMODE, // 0x20
        0x00,                                     // 0x0 act like ks0108
        SSD1306_SEGREMAP | 0x1, SSD1306_COMSCANDEC };
    ESP_ERR_ON_INIT_FAIL(ssd1306_write_commands(&(ssd1306->i2c_dev), (const uint8_t *)init3, ARRAY_SIZE(init3)));

    uint8_t contrast = 0x8F;
    if (display == DISP_w128xh32)
    {
        contrast = 0x8F;
    }
    else if (display == DISP_w128xh64)
    {
        contrast = (vccstate == SSD1306_EXTERNALVCC) ? 0x9F : 0xCF;
    }
    else if (display == DISP_w96xh16)
    {
        contrast = (vccstate == SSD1306_EXTERNALVCC) ? 0x10 : 0xAF;
    }
    else
    {
        // Other screen varieties -- TBD
    }
    uint8_t init4[]
        = { SSD1306_SETCONTRAST, contrast, SSD1306_SETPRECHARGE, ((vccstate == SSD1306_EXTERNALVCC) ? 0x22 : 0xF1) };

    ESP_ERR_ON_INIT_FAIL(ssd1306_write_commands(&(ssd1306->i2c_dev), (const uint8_t *)init4, ARRAY_SIZE(init4)));
    uint8_t init5[] = { SSD1306_SETVCOMDETECT, // 0xDB
        0x40,
        SSD1306_DISPLAYALLON_RESUME,                    // 0xA4
        SSD1306_NORMALDISPLAY,                          // 0xA6
        SSD1306_DEACTIVATE_SCROLL, SSD1306_DISPLAYON }; // Main screen turn on

    init5[3] = (ssd1306->invert) ? SSD1306_INVERTDISPLAY : SSD1306_NORMALDISPLAY;
    ESP_ERR_ON_INIT_FAIL(ssd1306_write_commands(&(ssd1306->i2c_dev), (const uint8_t *)init5, ARRAY_SIZE(init5)));

    // no need to check.
    ESP_ERR_ON_INIT_FAIL(ssd1306_clear_display(ssd1306));

    return ret;

END:
    ESP_ERROR_CHECK(ssd1306_deinit_desc(ssd1306));
    return ret;
}

esp_err_t ssd1306_deinit_desc(ssd1306_t *ssd1306)
{
    ESP_PARAM_CHECK(ssd1306);
    if (ssd1306->dispbuf)
    {
        free(ssd1306->dispbuf);
        ssd1306->dispbuf = NULL;
        ssd1306->dispbuflen = 0;
    }
    ESP_ERROR_CDEBUG(i2cdev_done());
    return (i2c_dev_delete_mutex(&(ssd1306->i2c_dev)));
}
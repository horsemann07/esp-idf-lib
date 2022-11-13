
// ssd1306 examples

#include <esp_err.h>
#include <esp_log.h>
#include <ssd1306.h>

#define TAG (__FILENAME__)

#ifndef APP_CPU_NUM
#define APP_CPU_NUM PRO_CPU_NUM
#endif

/* heltec wifi lora device
 * PIN CONFIGURATION
 *              | OLED |
 *  SCL         | 15   |
 *  SDA         | 04   |
 *  RST         | 16   |
 *  Vext_Ctrl   | 21   |
 */

#define HELTEC_OLED_SCL (15)
#define HELTEC_OLED_SDA (04)
#define HELTEC_OLED_RST (16)
#define I2C_PORT_NUM    (01)

/**
 * @brief
 * @param pvParameters
 */
void ssd1306_test(void *pvParameters)
{
    (void)pvParameters;
    esp_err_t ret = ESP_FAIL;

    ssd1306_t ssd1306;
    while (ret != ESP_OK)
    {
        ret = ssd1306_init_desc(&(ssd1306), HELTEC_OLED_SDA, HELTEC_OLED_SCL, I2C_PORT_NUM, HELTEC_OLED_RST, false,
            DISP_w128xh32, 0);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "gfx not initilized error %s", esp_err_to_name(ret));
        }
        vTaskDelay(1000 / portTICK_RATE_MS);
    }

    ESP_LOGI(TAG, "gfx initilized.");

    while (1)
    {
        ret = ssd1306_set_brightness(&(ssd1306), 100);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "failed to set brightness to 100");
        }
        vTaskDelay(pdMS_TO_TICKS(10000));
        ret = ssd1306_set_brightness(&(ssd1306), 0);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "failed to set brightness to 0");
        }
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

void app_main()
{
    ESP_ERROR_CHECK(i2cdev_init());
    xTaskCreatePinnedToCore(ssd1306_test, "ssd1306_test", configMINIMAL_STACK_SIZE * 15, NULL, 5, NULL, APP_CPU_NUM);
}

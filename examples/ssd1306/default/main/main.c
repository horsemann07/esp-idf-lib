
// ssd1306 examples

#include <esp_err.h>
#include <esp_log.h>
#include <ssd1306.h>

#define TAG (__FILENAME__)

#ifndef APP_CPU_NUM
#define APP_CPU_NUM PRO_CPU_NUM
#endif

void ssd1306_test(void *pvParameters)
{
    (void)pvParameters;
    esp_err_t ret = ESP_FAIL;

    ssd1306_gfx_t gfx = { 0 };
    while (ret != ESP_OK)
    {
        ret = ssd1306_gfx_init_desc(&(gfx), 22, 21, 1, 15, false, COLOR_WHITE, 0);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "gfx not initilized.");
        }
    }

    ESP_LOGI(TAG, "gfx initilized.");

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

void app_main()
{
    ESP_ERROR_CHECK(i2cdev_init());
    xTaskCreatePinnedToCore(ssd1306_test, "ssd1306_test", configMINIMAL_STACK_SIZE * 15, NULL, 5, NULL, APP_CPU_NUM);
}

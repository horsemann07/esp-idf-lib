/* standard C library */
#include <stdio.h>

/* freeRTOS library */
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

/* esp-idf headers */
#include <driver/gpio.h>
#include <esp_utils.h>

/* mq sensor header */
#include "mq.h"

#define TAG "mq2_test"

#define MQ_DIGI_PIN_IO  19
#define MQ_DIGI_PIN_SEL ((1ULL << MQ_DIGI_PIN_IO))

static xQueueHandle gpio_evt_queue = NULL;

static void IRAM_ATTR mq_digi_isr_handle(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void mq_digital_pin_test(void *arg)
{
    uint32_t io_num;
    for (;;)
    {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
        {
            ESP_LOGI(TAG, "GPIO[%d] intr, val: %d", io_num, gpio_get_level(io_num));
        }
    }
}

void mq2_test(void *pvParameters)
{
    (void)pvParameters;
    esp_err_t ret = ESP_OK;

    /* mq2 sensor config */
    mq_sensor_config_t mq2_sensor_config = MQ2_SENSOR_CONFIG_DEFAULT();
    mq2_sensor_config.mq_id = CONFIG_EXAMPLE_MQ_ID;

    /* init mq sensor init */
    ret = mq_sensor_init(&(mq2_sensor_config));
    ESP_ERROR_CHECK(ret);

    while (1)
    {
        mq_data_t data = { 0 };
        mq_sensor_read(&(mq2_sensor_config), &(data));

        float lpg = data.id.mq2.lpg;
        float co = data.id.mq2.co;
        float propane = data.id.mq2.propane;
        float smoke = data.id.mq2.smoke;

        if ((lpg > 1000) || (co > 1000) || (smoke > 1000) || (propane > 1000))
        {
            ESP_LOGI(TAG, "<Impure AIR>");
            ESP_LOGI(TAG, "lpg: %.2fppm \tco: %.2fppm \tsmoke: %.2fppm \tpropane: %0.2f ppm", lpg, co, smoke, propane);
        }

        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

esp_err_t mq_sensor_gpio_pin(gpio_num_t digi_pin)
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_POSEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << digi_pin),
        .pull_up_en = 1,
        .pull_down_en = 0,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // mq gpio intrrupt type for digital pin
    gpio_set_intr_type(digi_pin, GPIO_INTR_POSEDGE);

    // create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(5, sizeof(uint32_t));
    ESP_NULL_CHECK(gpio_evt_queue);

    // install gpio isr service
    ESP_ERROR_CHECK(gpio_install_isr_service(0));

    // hook isr handler for specific gpio pin
    ESP_ERROR_CHECK(gpio_isr_handler_add(digi_pin, mq_digi_isr_handle, (void *)digi_pin));
    return ESP_OK;
}

void app_main()
{
    mq_sensor_gpio_pin(MQ_DIGI_PIN_IO);

    assert(xTaskCreate(mq_digital_pin_test, "mq2_digi_test", 1024 * 3, NULL, 5, NULL) == pdPASS);
    assert(xTaskCreate(mq2_test, "mq2_test", 1024 * 3, NULL, 5, NULL) == pdPASS);

    return;
}

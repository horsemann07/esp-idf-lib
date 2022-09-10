/* ADC1 Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <esp_utils.h>

#include "adc_dev.h"

#define TAG __FILENAME__

static esp_adc_cal_characteristics_t *adc_chars = NULL;

static void check_efuse(void)
{
#if CONFIG_IDF_TARGET_ESP32
    // Check if TP is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK)
    {
        ESP_LOGD(TAG, "eFuse Two Point: Supported\n");
    }
    else
    {
        ESP_LOGD(TAG, "eFuse Two Point: NOT supported\n");
    }
    // Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK)
    {
        ESP_LOGD(TAG, "eFuse Vref: Supported\n");
    }
    else
    {
        ESP_LOGD(TAG, "eFuse Vref: NOT supported\n");
    }
#elif CONFIG_IDF_TARGET_ESP32S2
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK)
    {
        ESP_LOGD(TAG, "eFuse Two Point: Supported\n");
    }
    else
    {
        ESP_LOGD(TAG, "Cannot retrieve eFuse Two Point calibration values. Default calibration values will be used.\n");
    }
#else
#error "This adc dev is configured for ESP32/ESP32S2."
#endif
}

static void print_char_val_type(esp_adc_cal_value_t val_type)
{
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP)
    {
        ESP_LOGI(TAG, "Characterized using Two Point Value.");
    }
    else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF)
    {
        ESP_LOGI(TAG, "Characterized using eFuse Vref.");
    }
    else
    {
        ESP_LOGI(TAG, "Characterized using Default Vref.");
    }
}

uint32_t adc_dev_get_width_value(adc_bits_width_t width)
{
    return (pow(2, (width + 9)));
}

static uint32_t adc_dev_convert_raw_to_volt(uint32_t *raw_data)
{
    /* Convert adc_reading to voltage in mV */
    uint32_t adc_voltage = esp_adc_cal_raw_to_voltage((*raw_data), adc_chars);
    return adc_voltage;
}

uint32_t adc_dev_get_raw(const adc_dev_config_t *adc_setting)
{
    uint32_t adc_raw = 0;
    for (size_t i = 0; i < adc_setting->num_smple; i++)
    {
        if (adc_setting->unit == ADC_UNIT_1)
        {
            adc_raw += adc1_get_raw((adc1_channel_t)adc_setting->channel);
        }
        else
        {
            int raw;
            adc2_get_raw((adc2_channel_t)adc_setting->channel, adc_setting->width, &raw);
            adc_raw += raw;
        }
    }
    adc_raw /= adc_setting->num_smple;

    return adc_raw;
}

uint32_t adc_dev_get_volt_mV(const adc_dev_config_t *adc_setting)
{

    uint32_t adc_voltage = 0;
    uint32_t adc_reading = adc_dev_get_raw(adc_setting);

    /* Convert adc_reading to voltage in mV */
    adc_voltage = adc_dev_convert_raw_to_volt(&(adc_reading));
    return adc_voltage;
}

esp_err_t adc_dev_get_data(const adc_dev_config_t *adc_setting, adc_dev_data_t *const adc_data)
{
    ESP_PARAM_CHECK(adc_setting);
    ESP_PARAM_CHECK(adc_data);

    adc_data->raw_data = adc_dev_get_raw(adc_setting);
    adc_data->voltage = 0;

    /* Convert adc_reading to voltage in mV */
    adc_data->voltage = esp_adc_cal_raw_to_voltage(adc_data->raw_data, adc_chars);
    ESP_LOGD(TAG, "Raw: %u\tVoltage: %umV\n", adc_data->raw_data, adc_data->voltage);
    return ESP_OK;
}

esp_err_t adc_dev_init(adc_dev_config_t *const adc_setting)
{
    ESP_PARAM_CHECK(adc_setting);
    esp_err_t ret = ESP_OK;

    /* Check if Two Point or Vref are burned into eFuse */
    check_efuse();

    /* Configure ADC */
    if (adc_setting->unit == ADC_UNIT_1)
    {
        ret = adc1_config_width(adc_setting->width);
        ESP_ERROR_RETURN(ret != ESP_OK, ret, "failed to config adc1 width. ");
        ret = adc1_config_channel_atten(adc_setting->channel, adc_setting->atten);
        ESP_ERROR_RETURN(ret != ESP_OK, ret, "failed to config adc1 atten channel. ");
        ret = adc1_pad_get_io_num(adc_setting->channel, &(adc_setting->adc_pin));
        ESP_ERROR_PRINT(ret != ESP_OK, ret, "failed to get adc1 pin for channel %d.", adc_setting->channel);
        ESP_LOGI(TAG, "Analog pin %d", adc_setting->adc_pin);
    }
    else
    {
        ret = adc2_config_channel_atten((adc2_channel_t)adc_setting->channel, adc_setting->atten);
        ESP_ERROR_RETURN(ret != ESP_OK, ret, "failed to config adc1 atten channel. ");
        ret = adc2_pad_get_io_num(adc_setting->channel, &(adc_setting->adc_pin));
        ESP_ERROR_PRINT(ret != ESP_OK, ret, "failed to get adc2 pin for channel %d.", adc_setting->channel);
        ESP_LOGI(TAG, "Analog pin %d", adc_setting->adc_pin);
    }

    /* Characterize ADC */
    adc_chars = ESP_CALLOC(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(adc_setting->unit, adc_setting->atten, adc_setting->width,
        adc_setting->adc_volt_ref, adc_chars);

    print_char_val_type(val_type);
    return ESP_OK;
}

esp_err_t adc_dev_deinit(adc_dev_config_t *adc_setting)
{
    ESP_PARAM_CHECK(adc_setting);
    if (adc_chars != NULL)
    {
        ESP_FREE(adc_chars);
    }

    memset((void *)adc_setting, 0x00, sizeof(adc_dev_config_t));
    return ESP_OK;
}
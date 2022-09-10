

#ifndef _ADC_DEV_H_
#define _ADC_DEV_H_

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /* _GNU_SOURCE */

#include <driver/adc.h>
#include <driver/gpio.h>

#define ADC_DEFAULT_VREF    1100 // Use adc2_vref_to_gpio() to obtain a better estimate
#define ADC_DEFAULT_SAMPLES 64

typedef struct
{
    gpio_num_t adc_pin;
    adc_channel_t channel;
    adc_bits_width_t width;
    adc_atten_t atten;
    adc_unit_t unit;
    uint32_t adc_volt_ref;
    uint32_t num_smple;
} adc_dev_config_t;

typedef struct
{
    uint32_t raw_data;
    uint32_t voltage;
} adc_dev_data_t;

/**
 * @brief Get the adc width range. It depend on tht adc bit width
 * 
 * 
 * @param width; esp adc bit width
 * @return 
 *      - adc width value  = 2 ^ width
 */
uint32_t adc_dev_get_width_value(adc_bits_width_t width);

/**
 * @brief: read the adc raw value in bit range format.
 * 
 * @param[in] adc_setting: adc dev configuration. 
 * @return 
 *      - adc raw value.  
 */
uint32_t adc_dev_get_raw(const adc_dev_config_t *adc_setting);

/**
 * @brief: get voltage by converting raw in voltage.
 * 
 * @param[in] adc_setting: adc dev configuration. 
 * @return 
 *      - adc voltage value. 
 */
uint32_t adc_dev_get_volt_mV(const adc_dev_config_t *adc_setting);

/**
 * @brief: read the adc data s
 * 
 * @param[in] adc_setting: adc dev configuration. 
 * @param[out] adc_data 
 * @return 
 *      - ESP_OK: adc read successfully.
 *      - ESP_ERR_INVALID_ARG: null pointer
 */
esp_err_t adc_dev_get_data(const adc_dev_config_t *adc_setting, adc_dev_data_t *const adc_data);

/**
 * @brief: Initialized the adc with given configuraion
 * 
 * @param[in] config: adc dev configuration.
 * @return 
 *      - ESP_OK: init successfully
 *      - ESP_ERR_INVALID_ARG: null pointer
 *      - ESP_NO_MEMORY: heap unavailable
 */
esp_err_t adc_dev_init(adc_dev_config_t *config);

/**
 * @brief: Deinit initalized adc configuration
 * 
 * @param[in] adc_setting: adc dev configuration.
 * @return 
 *      - ESP_OK: deinit successfully.
 *      - ESP_ERR_INVALID_ARG: NULL pointer
 *   
 */
esp_err_t adc_dev_deinit(adc_dev_config_t *adc_setting);

#endif /* _ADC_DEV_H_ */
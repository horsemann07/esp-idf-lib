
#ifndef _MQ_H_
#define _MQ_H_

#include "adc_dev.h"
#include <esp_utils.h>
#include <stdio.h>

typedef enum {
    MQ2 = 0,
    MQ3,
    MQ4,
    MQ5,
    MQ6,
    MQ7, // 5
    MQ8,
    MQ9,
    MQ131,
    MQ135,
    MQ136, // 10
    MQ137,
    MQ138,
    MQ214, // 13
    MAX_MQ_ID
} mq_sensor_id_t;

typedef enum {
    METHANE = 0,
    PROPANE,
    BUTANE,
    LPG,
    SMOKE,
    ALCOHOL, // 5
    ETHANOL,
    CNG,
    CO,
    HYDROGEN,
    OZONE,      //10
    MAX_GAS_TYPE    // 11
} gas_type_t;

typedef struct
{
    mq_sensor_id_t mq_id;
    float Ro;
    float Rs;
    float Rl;
    float clean_air_factor;
    uint8_t num_samples;
    
    adc_dev_config_t adc_config;
} mq_sensor_config_t;

typedef struct
{
    volatile adc_dev_data_t volatile_adc;
    union
    {
        struct mq2
        {
            uint32_t lpg;
            uint32_t propane;
            uint32_t co;
            uint32_t smoke;
        } mq2;

        struct mq3
        {
            uint32_t alcohol;
            uint32_t ethanol;
            uint32_t smoke;
        } mq3;

        struct mq4
        {
            uint32_t methane;
            uint32_t lpg;
        } mq4;

        struct mq5
        {
            uint32_t methane;
            uint32_t lpg;
        } mq5;

        struct mq6
        {
            uint32_t lpg;
            uint32_t methane;
            uint32_t hyderogen;
        } mq6;

        struct mq7
        {
            uint32_t co;
            uint32_t hyderogen;
        } mq7;

        struct mq8
        {
            uint32_t hyderogen;
        } mq8;

        struct mq9
        {
            uint32_t co;
            uint32_t lpg;
        } mq9;

    } id;

} mq_data_t;

#define MQ2_SENSOR_CONFIG_DEFAULT()                 \
    {                                               \
        .mq_id = MQ2,                                \
        .Ro = 10,                                   \
        .Rs = 0,                                    \
        .Rl = 5,                                    \
        .clean_air_factor = 9.8,                    \
        .num_samples = 5,                           \
        .adc_config =                               \
            {                                       \
                .channel = ADC_CHANNEL_0,           \
                .width = ADC_WIDTH_BIT_12,          \
                .atten = ADC_ATTEN_DB_0,            \
                .unit = ADC_UNIT_1,                 \
                .adc_volt_ref = ADC_DEFAULT_VREF,   \
                .num_smple = ADC_DEFAULT_SAMPLES,   \
            },                                      \
    }   

esp_err_t mq_sensor_read(const mq_sensor_config_t *mq_setting, mq_data_t *mq_data);
esp_err_t mq_callibrate(mq_sensor_config_t *const mq_setting);
/**
 * @brief configure mq2 sensor.
 *
 * @param[in] config    pointer to mq2 config.
 * @return
 *      - ESP_OK
 */
esp_err_t mq_sensor_init(mq_sensor_config_t *config);

/**
 * @brief deconfigure mq2 sensor.
 *
 * @param[in] config    pointer to mq2 config.
 * @return
 *      - ESP_OK
 */
esp_err_t mq_sensor_deinit(mq_sensor_config_t *config);

#endif /* _MQ_H_ */
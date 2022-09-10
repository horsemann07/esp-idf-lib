
#include <math.h>
#include "mq.h"

#define TAG __FILENAME__

/* mq2 gases curve point */
static float s_mq2_lpg_curve_pt[3] = { 2.3, 0.21, -0.47 };
static float s_mq2_propane_curve_pt[3] = { 2.3, 0.27, -0.46 };
static float s_mq2_co_curve_pt[3] = { 2.3, 0.72, -0.34 };
static float s_mq2_smoke_curve_pt[3] = { 2.3, 0.53, -0.44 };

/* mq3 gases curve point */
static float s_mq3_alohol_curve_pt[3] = { -1.00, 0.05, -0.46 };
static float s_mq3_ethanol_curve_pt[3] = { -1.00, 1.11, -0.93 };
static float s_mq3_smoke_curve_pt[3] = { -1.00, 1.70, -0.34 };

/* mq4 gases curve point */
static float s_mq4_methane_curve_pt[3] = { 2.30, 0.26, -0.35 };
static float s_mq4_lpg_curve_pt[3] = { 2.30, 1.26, -0.81 };

/* mq5 gases curve point */
static float s_mq5_methane_curve_pt[3] = { 2.30, -0.045, -0.45 };
static float s_mq5_lpg_curve_pt[3] = { 2.30, -0.22, -0.35 };

/* mq6 gases curve point */
static float s_mq6_lpg_curve_pt[3] = { 2.30, 0.30, -0.41 };
static float s_mq6_methane_curve_pt[3] = { 2.30, 0.44, -0.48 };
static float s_mq6_hyderogen_curve_pt[3] = { 2.30, 0.77, -0.28 };

/* mq7 gases curve point */
static float s_mq7_hyderogen_curve_pt[3] = { 1.69, 0.14, -0.81 };
static float s_mq7_co_curve_pt[3] = { 1.69, 0.14, -0.62 };

/* mq8 gases curve point */
static float s_mq8_hyderogen_curve_pt[3] = { 2.0, -0.60, -0.39 };

/* mq9 gases curve point */
static float s_mq9_co_curve_pt[3] = { 2.30, 0.255, -0.50 };
static float s_mq9_lpg_curve_pt[3] = { 2.30, 0.30, -0.56 };

static float mq_resistance_calculation(uint32_t width_val, float RL, uint32_t raw_adc)
{
    return ((RL * (width_val - ((int)raw_adc)) / ((int)raw_adc)));
}

static uint8_t mq_get_percentage(float rs_ro_ratio, float *pcurve)
{
    return (pow(10, (((log(rs_ro_ratio) - pcurve[1]) / pcurve[2]) + pcurve[0])));
}

static uint32_t mq_get_gas_percentage(float rs_ro_ratio, mq_sensor_id_t mq_id, gas_type_t gas_type)
{
    ESP_PARAM_CHECK(mq_id < MAX_MQ_ID);
    ESP_PARAM_CHECK(gas_type < MAX_GAS_TYPE);

    uint8_t percent = 0;

    switch (mq_id)
    {
        case MQ2:
            if (gas_type == LPG)
            {
                percent = mq_get_percentage(rs_ro_ratio, s_mq2_lpg_curve_pt);
            }
            else if (gas_type == PROPANE)
            {
                percent = mq_get_percentage(rs_ro_ratio, s_mq2_propane_curve_pt);
            }
            else if (gas_type == CO)
            {
                percent = mq_get_percentage(rs_ro_ratio, s_mq2_co_curve_pt);
            }
            // smoke
            else
            {
                percent = mq_get_percentage(rs_ro_ratio, s_mq2_smoke_curve_pt);
            }
            break;

        case MQ3:
            if (gas_type == ALCOHOL)
            {
                percent = mq_get_percentage(rs_ro_ratio, s_mq3_alohol_curve_pt);
            }
            else if (gas_type == ETHANOL)
            {
                percent = mq_get_percentage(rs_ro_ratio, s_mq3_ethanol_curve_pt);
            }
            // smoke
            else
            {
                percent = mq_get_percentage(rs_ro_ratio, s_mq3_smoke_curve_pt);
            }
            break;

        case MQ4:
            if (gas_type == METHANE)
            {
                percent = mq_get_percentage(rs_ro_ratio, s_mq4_methane_curve_pt);
            }
            // lpg
            else
            {
                percent = mq_get_percentage(rs_ro_ratio, s_mq4_lpg_curve_pt);
            }
            break;

        case MQ5:
            if (gas_type == METHANE)
            {
                percent = mq_get_percentage(rs_ro_ratio, s_mq5_methane_curve_pt);
            }
            // LPG
            else
            {
                percent = mq_get_percentage(rs_ro_ratio, s_mq5_lpg_curve_pt);
            }
            break;

        case MQ6:
            if (gas_type == LPG)
            {
                percent = mq_get_percentage(rs_ro_ratio, s_mq6_lpg_curve_pt);
            }
            else if (gas_type == METHANE)
            {
                percent = mq_get_percentage(rs_ro_ratio, s_mq6_methane_curve_pt);
            }
            // hyderogen
            else
            {
                percent = mq_get_percentage(rs_ro_ratio, s_mq6_hyderogen_curve_pt);
            }
            break;

        case MQ7:
            if (gas_type == CO)
            {
                percent = mq_get_percentage(rs_ro_ratio, s_mq7_co_curve_pt);
            }
            // hyderogen
            else
            {
                percent = mq_get_percentage(rs_ro_ratio, s_mq7_hyderogen_curve_pt);
            }
            break;

        case MQ8:
            // hyderogen
            percent = mq_get_percentage(rs_ro_ratio, s_mq8_hyderogen_curve_pt);
            break;

        case MQ9:
            if (gas_type == CO)
            {
                percent = mq_get_percentage(rs_ro_ratio, s_mq9_co_curve_pt);
            }
            // lpg
            else
            {
                percent = mq_get_percentage(rs_ro_ratio, s_mq9_lpg_curve_pt);
            }
            break;

        default:
            break;
    }
    return percent;
}

static float mq_get_callibrate_rs(const adc_dev_config_t *adc_setting, float RL, int samples)
{
    float rs_raw = 0;
    uint32_t adc_raw = 0;

    /* get adc width range */
    uint32_t adc_width_range = adc_dev_get_width_value(adc_setting->width);

    for (int i = 0; i < samples; i++)
    {
        adc_raw = adc_dev_get_raw(adc_setting);
        rs_raw += mq_resistance_calculation(adc_width_range, RL, adc_raw);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    rs_raw /= samples;
    return rs_raw;
}

static float mq_get_rs(const mq_sensor_config_t *mq_setting)
{
    float rs = mq_get_callibrate_rs(&(mq_setting->adc_config), mq_setting->Rl, mq_setting->adc_config.num_smple);
    return rs;
}

esp_err_t mq_sensor_read(const mq_sensor_config_t *mq_setting, mq_data_t *mq_data)
{
    ESP_PARAM_CHECK(mq_setting);
    ESP_PARAM_CHECK(mq_data);

    float rs_ro_ratio = mq_get_rs(mq_setting) / mq_setting->Ro;
    mq_sensor_id_t ID = mq_setting->mq_id;
    switch (ID)
    {
        case MQ2:
            mq_data->id.mq2.lpg = mq_get_gas_percentage(rs_ro_ratio, ID, LPG);
            mq_data->id.mq2.propane = mq_get_gas_percentage(rs_ro_ratio, ID, PROPANE);
            mq_data->id.mq2.co = mq_get_gas_percentage(rs_ro_ratio, ID, CO);
            mq_data->id.mq2.smoke = mq_get_gas_percentage(rs_ro_ratio, ID, SMOKE);
            break;

        case MQ3:
            mq_data->id.mq3.ethanol = mq_get_gas_percentage(rs_ro_ratio, ID, ETHANOL);
            mq_data->id.mq3.alcohol = mq_get_gas_percentage(rs_ro_ratio, ID, ALCOHOL);
            mq_data->id.mq3.smoke = mq_get_gas_percentage(rs_ro_ratio, ID, SMOKE);
            break;

        case MQ4:
            mq_data->id.mq4.methane = mq_get_gas_percentage(rs_ro_ratio, ID, METHANE);
            mq_data->id.mq4.lpg = mq_get_gas_percentage(rs_ro_ratio, ID, LPG);
            break;

        case MQ5:
            mq_data->id.mq5.lpg = mq_get_gas_percentage(rs_ro_ratio, ID, LPG);
            mq_data->id.mq5.methane = mq_get_gas_percentage(rs_ro_ratio, ID, METHANE);
            break;

        case MQ6:
            mq_data->id.mq6.lpg = mq_get_gas_percentage(rs_ro_ratio, ID, LPG);
            mq_data->id.mq6.methane = mq_get_gas_percentage(rs_ro_ratio, ID, METHANE);
            mq_data->id.mq6.hyderogen = mq_get_gas_percentage(rs_ro_ratio, ID, HYDROGEN);
            break;

        case MQ7:
            mq_data->id.mq7.co = mq_get_gas_percentage(rs_ro_ratio, ID, CO);
            mq_data->id.mq7.hyderogen = mq_get_gas_percentage(rs_ro_ratio, ID, HYDROGEN);
            break;

        case MQ8:
            mq_data->id.mq8.hyderogen = mq_get_gas_percentage(rs_ro_ratio, ID, HYDROGEN);
            break;

        case MQ9:
            mq_data->id.mq9.co = mq_get_gas_percentage(rs_ro_ratio, ID, CO);
            mq_data->id.mq9.lpg = mq_get_gas_percentage(rs_ro_ratio, ID, LPG);
            break;

        default:
            break;
    }
    return ESP_OK;
}

esp_err_t mq_callibrate(mq_sensor_config_t *const mq_setting)
{
    ESP_PARAM_CHECK(mq_setting);
    mq_setting->Ro = 0;
    float ro_raw = 0;
    uint32_t adc_raw = 0;
    float RL = mq_setting->Rl;

    /* get adc width range */
    uint32_t adc_width_range = adc_dev_get_width_value(mq_setting->adc_config.width);

    for (int i = 0; i < mq_setting->num_samples; i++)
    {
        adc_raw = adc_dev_get_raw(&(mq_setting->adc_config));
        ro_raw += mq_resistance_calculation(adc_width_range, RL, adc_raw);
        ESP_LOGV(TAG, "%d: %0.2f", i, ro_raw);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    ro_raw /= mq_setting->num_samples;
    ro_raw /= mq_setting->clean_air_factor;

    mq_setting->Ro = ro_raw;
    return ESP_OK;
}

esp_err_t mq_sensor_init(mq_sensor_config_t *config)
{
    ESP_PARAM_CHECK(config);
    esp_err_t ret = ESP_OK;

    /* init adc */
    ret = adc_dev_init(&(config->adc_config));
    ESP_ERROR_RETURN(ret != ESP_OK, ret, "failed to init adc dev");

    ESP_LOGI(TAG, "Keep sensor in clean air");
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    ESP_LOGI(TAG, "Callibrating Ro....");

    /* callibrate Ro value */
    ret = mq_callibrate(config);
    ESP_ERROR_RETURN(ret != ESP_OK, ret, "Failed to callibrate Ro");
    ESP_LOGI(TAG, "Callibration Done! RO: %0.2f kOhm", config->Ro);

    return ret;
}
idf_component_register(
    SRCS "adc_dev.c" "mq.c"
    INCLUDE_DIRS .
    REQUIRES log esp_idf_lib_helpers utils driver esp_adc_cal newlib
)

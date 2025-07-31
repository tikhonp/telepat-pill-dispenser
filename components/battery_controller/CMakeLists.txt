idf_component_register(SRCS "battery_controller.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES esp_adc esp_driver_gpio)

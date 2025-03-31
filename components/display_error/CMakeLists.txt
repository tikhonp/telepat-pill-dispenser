idf_component_register(SRCS "display_error.c" "led_controller.c" "sleep_controller.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES global_manager buzzer pilld_models
                    led_indicator button_controller)

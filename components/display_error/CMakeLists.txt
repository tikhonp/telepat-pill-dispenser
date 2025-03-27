idf_component_register(SRCS "display_error.c" "led_controller.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES global_manager buzzer
                    led_indicator button_controller)

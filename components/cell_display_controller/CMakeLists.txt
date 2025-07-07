idf_component_register(SRCS "cell_activity_ws2812b.c" "cell_activity_watchdog.c" "cell_led_plain_pins_controller.c" "cell_led_matrix_controller.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES 
                    buzzer pilld_models global_manager display_error 
                    button_controller esp_timer medsenger_http_requests 
                    esp_driver_gpio ht16k33 led_strip)
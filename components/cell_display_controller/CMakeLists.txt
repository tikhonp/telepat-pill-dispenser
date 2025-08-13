idf_component_register(SRCS "cell_activity_ws2812b.c" "cell_activity_watchdog.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES 
                    buzzer pilld_models global_manager display_error 
                    button_controller esp_timer medsenger_http_requests 
                    esp_driver_gpio ht16k33 led_strip ws2812b_controller)
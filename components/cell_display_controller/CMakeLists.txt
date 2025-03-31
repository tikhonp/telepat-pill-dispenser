idf_component_register(SRCS "cell_activity_watchdog.c" "cell_led_controller.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES 
                    buzzer schedule_data global_manager display_error 
                    button_controller esp_timer medsenger_http_requests 
                    esp_driver_gpio)

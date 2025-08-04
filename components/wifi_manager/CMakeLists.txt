idf_component_register(SRCS "wifi_manager.c" "connect.c" "time_sync.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES esp_wifi global_manager captive_portal display_error cell_display_controller freertos esp_timer esp_pm esp_system)

idf_component_register(SRCS "wifi_manager.c" "connect.c" "time_sync.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES esp_wifi)

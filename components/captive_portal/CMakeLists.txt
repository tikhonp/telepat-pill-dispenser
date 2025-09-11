idf_component_register(SRCS "captive_portal.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES esp_wifi nvs_flash button_controller cleaner
                    REQUIRES esp_http_server)

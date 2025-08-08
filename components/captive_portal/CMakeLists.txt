idf_component_register(SRCS "captive_portal.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES esp_wifi nvs_flash
                    REQUIRES esp_http_server display_error)
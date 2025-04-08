idf_component_register(SRCS "buzzer.c" "buzzer_player.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES esp_driver_ledc esp_driver_gpio pilld_common)

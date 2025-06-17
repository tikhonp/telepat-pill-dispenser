idf_component_register(SRCS "init_global_manager.c" "medsenger_synced.c" "stop_all_tasks.c" "wifi_creds.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES nvs_flash)

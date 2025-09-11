idf_component_register(SRCS "schedule_data.c" "send_event_data.c" "medsenger_refresh_rate.c" "last_confirmed_cell.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES nvs_flash global_manager)

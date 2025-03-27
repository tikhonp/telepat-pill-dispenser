idf_component_register(SRCS "cell_display_controller.c" "cell_activity_watchdog.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES schedule_data global_manager display_error)

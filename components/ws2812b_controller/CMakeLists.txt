idf_component_register(
    SRCS "ws2812b_controller.c"
    INCLUDE_DIRS "include"
    REQUIRES led_strip driver esp_timer
)

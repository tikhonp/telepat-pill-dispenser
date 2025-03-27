set(requires nvs_flash esp_event esp_netif esp_driver_gpio)

idf_component_register(
  SRCS "telepat-pill-dispenser.c" "scheduler.c" "cells.c"
  PRIV_REQUIRES
  ${requires} global_manager wifi_manager schedule_data
  pilld_common medsenger_http_requests buzzer display_error
  button_controller
  INCLUDE_DIRS ".")

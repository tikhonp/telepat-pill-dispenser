set(requires nvs_flash esp_event esp_netif)

idf_component_register(
  SRCS "telepat-pill-dispenser.c" "scheduler.c" "button.c" "cells.c"
  PRIV_REQUIRES
  ${requires} global_manager wifi_manager schedule_data
  pilld_common medsenger_http_requests buzzer
  INCLUDE_DIRS ".")

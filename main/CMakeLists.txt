set(requires esp-tls nvs_flash esp_event esp_netif esp_http_client esp_wifi)

idf_build_get_property(target IDF_TARGET)
if(${target} STREQUAL "linux")
    list(
    APPEND
    requires
    esp_stubs
    protocol_examples_common
    esp_netif_stack)
endif()

set(srcs "telepat-pill-dispenser.c" "schedule-request.c"
            "schedule-storage.c" "scheduler.c" "button.c" "submit-request.c")
if(NOT ${target} STREQUAL "linux")
    list(APPEND srcs "cells.c")
endif()

idf_component_register(
  SRCS
  ${srcs}
  PRIV_REQUIRES
  ${requires} global_manager wifi_manager schedule_data
  INCLUDE_DIRS
  "."
  EMBED_TXTFILES
  howsmyssl_com_root_cert.pem)

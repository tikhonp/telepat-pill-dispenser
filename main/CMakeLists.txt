idf_build_get_property(target IDF_TARGET)

set(requires "")
if(${target} STREQUAL "linux")
    list(APPEND requires esp_stubs esp_event esp-tls esp_http_client protocol_examples_common nvs_flash esp_netif esp_netif_stack)
endif()

set(sources "telepat-pill-dispenser.c" "schedule-request.c" "protocols-common.c" "schedule-storage.c" "scheduler.c")
if (NOT ${target} STREQUAL "linux")
    list(APPEND sources "cells.c" "wifi.c")
endif()

idf_component_register(SRCS ${sources} REQUIRES ${requires} INCLUDE_DIRS "." EMBED_TXTFILES howsmyssl_com_root_cert.pem)

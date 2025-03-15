set(requires "")
idf_build_get_property(target IDF_TARGET)

if(${target} STREQUAL "linux")
    list(APPEND requires esp_stubs esp_event esp-tls esp_http_client protocol_examples_common nvs_flash esp_wifi driver)
endif()

idf_component_register(SRCS "telepat-pill-dispenser.c" "wifi.c" "schedule-request.c" "protocols-common.c" "schedule-storage.c" "scheduler.c" "cells.c"
                    REQUIRES ${requires}
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES howsmyssl_com_root_cert.pem)

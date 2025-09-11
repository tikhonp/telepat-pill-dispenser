#pragma once

#include "esp_err.h"
#include <stddef.h>
#include <stdint.h>

#pragma pack(push, 1)
typedef struct {
    uint32_t timestamp;
    uint8_t cell_indx;
} se_send_event_t;
#pragma pack(pop)

esp_err_t se_add_fsent_event(se_send_event_t event);

esp_err_t se_get_fsent_events(se_send_event_t *buf, size_t buf_size,
                              size_t *elements_count);

esp_err_t se_save_fsent_events(se_send_event_t *buf, size_t events_count);

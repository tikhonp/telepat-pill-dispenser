#pragma once

#include "esp_err.h"

esp_err_t save_last_confirmed_cell_intrv(uint32_t timestamp_start,
                                         uint32_t timestamp_end);
esp_err_t load_last_confirmed_cell_intrv(uint32_t *timestamp_start,
                                         uint32_t *timestamp_end);

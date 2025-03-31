#pragma once

#include "esp_err.h"
#include <stdint.h>

esp_err_t m_get_medsenger_refresh_rate_sec(uint32_t *refresh_rate);

esp_err_t m_save_medsenger_refresh_rate_sec(uint32_t refresh_rate);

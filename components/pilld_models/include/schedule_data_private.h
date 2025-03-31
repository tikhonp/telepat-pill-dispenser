#pragma once

#include "esp_err.h"

esp_err_t sd_save_to_flash(char *buf, unsigned int buf_length);
esp_err_t sd_save_schedule_to_memory(void *buf, size_t buf_length);

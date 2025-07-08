#pragma once

#include "esp_err.h"

/**
 * @brief Полностью очищает NVS раздел.
 * 
 * Стирает все данные во всех пространствах имён NVS.
 *
 * @return esp_err_t ESP_OK при успехе, иначе ошибка из esp_err.h
 */
esp_err_t nvs_clean_all(void);

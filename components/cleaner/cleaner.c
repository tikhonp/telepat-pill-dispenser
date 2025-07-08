#include "cleaner.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: export

static const char *TAG = "cleaner";

esp_err_t nvs_clean_all(void) {
    esp_err_t err;

    ESP_LOGI(TAG, "Clining started...");
    err = nvs_flash_erase();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Got cleaning problem: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Cleaned, rebooting in 0.5s...");
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();

    return ESP_OK;
}

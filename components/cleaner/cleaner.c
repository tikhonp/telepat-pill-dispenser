#include "cleaner.h"
#include "esp_log.h"
#include "mfg_data.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: export
#include "wifi_creds.h"

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

void flash_default_wifi_credentials(void) {
    wifi_creds_t creds;
    ESP_ERROR_CHECK(read_default_wifi_creds(creds.ssid, SSID_BUF_LEN, creds.psk, PSK_BUF_LEN));
    ESP_LOGI(TAG, "Flashing default Wi-Fi credentials: ssid='%s', psk='%s'",
             creds.ssid, creds.psk);
    ESP_ERROR_CHECK(gm_set_wifi_creds(&creds));
    ESP_LOGI(TAG, "Default Wi-Fi credentials flashed");
    esp_restart();
}

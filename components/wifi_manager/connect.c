#include "connect.h"
#include "esp_netif_sntp.h"
#include "esp_system.h"
#include "time_sync_private.h"
#include "wifi_manager_private.h"

esp_err_t connect(void) {
    if (wifi_connect() != ESP_OK) {
        return ESP_FAIL;
    }
    ESP_ERROR_CHECK(esp_register_shutdown_handler(&wifi_shutdown));
    obtain_time();
    return ESP_OK;
}

esp_err_t disconnect(void) {
    wifi_shutdown();
    ESP_ERROR_CHECK(esp_unregister_shutdown_handler(&wifi_shutdown));
    esp_netif_sntp_deinit();
    return ESP_OK;
}

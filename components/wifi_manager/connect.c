#include "connect.h"
#include "esp_netif_sntp.h"
#include "esp_system.h"
#include "time_sync_private.h"
#include "wifi_manager_private.h"

esp_err_t wm_connect(void) {
    if (wm_wifi_connect() != ESP_OK) {
        return ESP_FAIL;
    }
    ESP_ERROR_CHECK(esp_register_shutdown_handler(&wm_wifi_shutdown));
    wm_obtain_time();
    return ESP_OK;
}

esp_err_t wm_disconnect(void) {
    wm_wifi_shutdown();
    ESP_ERROR_CHECK(esp_unregister_shutdown_handler(&wm_wifi_shutdown));
    esp_netif_sntp_deinit();
    return ESP_OK;
}

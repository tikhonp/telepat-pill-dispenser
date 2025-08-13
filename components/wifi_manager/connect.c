#include "connect.h"
#include "captive_portal.h"
#include "esp_err.h"
#include "esp_netif_sntp.h"
#include "esp_system.h"
#include "sdkconfig.h"
#include "time_sync_private.h"
#include "wifi_creds.h"
#include "wifi_manager_private.h"
#include <string.h>
#include <esp_log.h>

static const char *TAG = "wifi_manager";

esp_err_t wm_connect() {
    wifi_creds_t creds;
    esp_err_t err = gm_get_wifi_creds(&creds);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "wm_connect: no credentials found (err: %s)", esp_err_to_name(err));
    }
    if (err != ESP_OK) {
        wifi_credentials_t obtained_creds = start_wifi_captive_portal(
            CONFIG_WM_CAPTIVE_PORTAL_NETWORK_NAME, "");
        // TODO: check creds.success
        strncpy(creds.ssid, obtained_creds.ssid, 33);
        strncpy(creds.psk, obtained_creds.psk, 33);

        ESP_ERROR_CHECK(gm_set_wifi_creds(&creds));
    }
    if (wm_wifi_connect(creds) != ESP_OK) {
        return ESP_FAIL;
    }
    ESP_ERROR_CHECK(esp_register_shutdown_handler(&wm_wifi_shutdown));
    wm_obtain_time();
    return ESP_OK;
}

esp_err_t wm_disconnect(void) {
    wm_wifi_shutdown();
    // ESP_ERROR_CHECK(esp_unregister_shutdown_handler(&wm_wifi_shutdown));
    esp_netif_sntp_deinit();
    return ESP_OK;
}

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
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_sleep.h"
#include "esp_timer.h"
#include "cell_led_controller.h"
// debug blink task handle
static TaskHandle_t debug_blink_handle = NULL;

// Blink all LEDs on/off
static void debug_blink_task(void *arg) {
    bool on = false;
    while (1) {
        for (uint8_t i = 0; i < CONFIG_SD_CELLS_COUNT; i++) {
            if (on) cdc_enable_signal(i);
            else cdc_disable_signal(i);
        }
        on = !on;
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

esp_err_t wm_connect(bool extended_debug) {
    wifi_creds_t creds;
    esp_err_t err = gm_get_wifi_creds(&creds);
    if (err != ESP_OK) {
        wifi_credentials_t obtained_creds = start_wifi_captive_portal(
            CONFIG_WM_CAPTIVE_PORTAL_NETWORK_NAME, "");
        // TODO: check creds.success
        strncpy(creds.ssid, obtained_creds.ssid, 33);
        strncpy(creds.psk, obtained_creds.psk, 33);

        ESP_ERROR_CHECK(gm_set_wifi_creds(&creds));
    }
    if (extended_debug) {
        // reinit LED strip after wake
        cdc_deinit_led_signals();
        cdc_init_led_signals();
        xTaskCreate(debug_blink_task, "dbg-blink", 2048, NULL, 5, &debug_blink_handle);
    }
    esp_err_t conn_ret = wm_wifi_connect(creds);
    if (extended_debug) {
        // stop loading blink
        if (debug_blink_handle) vTaskDelete(debug_blink_handle);
        // final color blink for 3s
        uint32_t end = esp_timer_get_time() + 3000000;
        bool color_on = true;
        while (esp_timer_get_time() < end) {
            // set color: green for success, red for error
            for (uint8_t i = 0; i < CONFIG_SD_CELLS_COUNT; i++) {
                if (conn_ret == ESP_OK && color_on) cdc_enable_signal(i);
                else if (conn_ret != ESP_OK && color_on) cdc_enable_signal(i);
                else cdc_disable_signal(i);
            }
            color_on = !color_on;
            vTaskDelay(pdMS_TO_TICKS(500));
        }
        cdc_deinit_led_signals();
    }
    if (conn_ret != ESP_OK) {
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

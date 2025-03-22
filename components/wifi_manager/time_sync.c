#include "esp_log.h"
#include "esp_netif_sntp.h"
#include "esp_sntp.h"
#include <sys/time.h>

static const char *TAG = "time-sync";

static void wm_time_sync_notification_cb(struct timeval *tv) {
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

static void wm_print_servers(void) {
    ESP_LOGI(TAG, "List of configured NTP servers:");

    for (uint8_t i = 0; i < SNTP_MAX_SERVERS; ++i) {
        if (esp_sntp_getservername(i)) {
            ESP_LOGI(TAG, "server %d: %s", i, esp_sntp_getservername(i));
        }
    }
}

void wm_obtain_time(void) {
    ESP_LOGI(TAG, "Initializing and starting SNTP");
    esp_sntp_config_t config =
        ESP_NETIF_SNTP_DEFAULT_CONFIG(CONFIG_WM_SNTP_TIME_SERVER);
    config.sync_cb =
        wm_time_sync_notification_cb; // Note: This is only needed if we want

    esp_netif_sntp_init(&config);
    wm_print_servers();

    // wait for time to be set
    int retry = 0;
    const int retry_count = 15;
    while (esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) ==
               ESP_ERR_TIMEOUT &&
           ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry,
                 retry_count);
    }
}

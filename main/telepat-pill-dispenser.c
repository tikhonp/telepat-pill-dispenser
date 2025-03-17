#include "button.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: export
#include "nvs_flash.h"
#include "schedule-request.h"
#include "schedule-storage.h"
#include "scheduler.h"
#include "sdkconfig.h"
#include <stdint.h>
#if !CONFIG_IDF_TARGET_LINUX
#include "cells.h"
#include "esp_netif.h"
#include "wifi.h"
#endif

#define TAG "telepat-pill-dispenser"

#define LED_PIN 2

void schedule_handler(uint32_t *timestamps) {
    for (int i = 0; i < CONFIG_CELLS_COUNT; i++)
        ESP_LOGI("HNDLR", "TIMESTAMP: %" PRIu32, timestamps[i]);

    update_schedule(timestamps);
}

void app_main(void) {
    ESP_LOGI(TAG, "Hello world!");

    init_schedule();
#if !CONFIG_IDF_TARGET_LINUX
    init_cells();
#endif
    button_init();

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

#if !CONFIG_IDF_TARGET_LINUX
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();
#endif
    sync_time();

    run_fetch_schedule_task(&schedule_handler);
    run_scheduler_task();

    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

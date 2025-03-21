#include "button.h"
#include "esp_err.h"
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
#include "connect.h"
#include "esp_netif.h"
#endif
#include "init_global_manager.h"
#include "medsenger_synced.h"

#define TAG "telepat-pill-dispenser"

#define LED_PIN 2


/* Variable holding number of times ESP32 restarted since first boot.
 * It is placed into RTC memory using RTC_DATA_ATTR and
 * maintains its value when ESP32 wakes from deep sleep.
 */
RTC_DATA_ATTR static int boot_count = 0;

static void main_flow(void) {
    ESP_LOGI(TAG, "Starting pill-dispenser...");

    // Initialize NVS, network and freertos
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    init_global_manager();

    if (connect() != ESP_OK) {
        set_medsenger_synced(false);
    }
}

void schedule_handler(uint32_t *timestamps) {
    for (int i = 0; i < CONFIG_CELLS_COUNT; i++)
        ESP_LOGI("HNDLR", "TIMESTAMP: %" PRIu32, timestamps[i]);

    update_schedule(timestamps);
}

void app_main(void) {
    ++boot_count;
    ESP_LOGI(TAG, "Boot count: %d", boot_count);

    main_flow();

    init_schedule();
    init_cells();
    button_init();

    run_fetch_schedule_task(&schedule_handler);
    run_scheduler_task();

    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

#include "button.h"
#include "cells.h"
#include "connect.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: export
#include "init_global_manager.h"
#include "medsenger_http_requests.h"
#include "medsenger_synced.h"
#include "nvs_flash.h"
#include "schedule_data.h"
#include "scheduler.h"
#include <stdint.h>

#define TAG "telepat-pill-dispenser"

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

    sd_init();
    gm_init();

    if (wm_connect() != ESP_OK) {
        gm_set_medsenger_synced(false);
        ESP_LOGE(TAG, "Failed to connect to wi-fi");
        ESP_ERROR_CHECK(sd_load_schedule_from_flash());
    } else if (mhr_fetch_schedule(&sd_save_schedule) != ESP_OK) {
        gm_set_medsenger_synced(false);
        ESP_LOGE(TAG, "Failed to fetch medsenger schedule");
        ESP_ERROR_CHECK(sd_load_schedule_from_flash());
    }
    sd_print_schedule();
}

void app_main(void) {
    ++boot_count;
    ESP_LOGI(TAG, "Boot count: %d", boot_count);

    main_flow();

    init_cells();
    button_init();
    run_scheduler_task();

    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

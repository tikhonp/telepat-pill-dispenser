#include "button_controller.h"
#include "buzzer.h"
#include "cells.h"
#include "connect.h"
#include "display_error.h"
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
#include <sys/time.h>

#define TAG "telepat-pill-dispenser"

/* Variable holding number of times ESP32 restarted since first boot.
 * It is placed into RTC memory using RTC_DATA_ATTR and
 * maintains its value when ESP32 wakes from deep sleep.
 */
RTC_DATA_ATTR static int boot_count = 0;

static void main_flow(void) {
    ESP_LOGI(TAG, "Starting pill-dispenser...");

    // Initialize NVS, network and freertos
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    sd_init();
    gm_init();
    b_init();
    bc_init();

    if (wm_connect() != ESP_OK) {
        gm_set_medsenger_synced(false);
        ESP_LOGE(TAG, "Failed to connect to wi-fi");
        err = sd_load_schedule_from_flash();
        if (err != ESP_OK) {
            de_display_error(FLASH_LOAD_FAILED);
        }
    } else if (mhr_fetch_schedule(&sd_save_schedule) != ESP_OK) {
        gm_set_medsenger_synced(false);
        ESP_LOGE(TAG, "Failed to fetch medsenger schedule");
        err = sd_load_schedule_from_flash();
        if (err != ESP_OK) {
            de_display_error(FLASH_LOAD_FAILED);
        }
    }
    sd_print_schedule();
}

static void button_task(void *params) {
    while (true) {
        bc_wait_for_single_press();
        struct timeval tv;
        gettimeofday(&tv, NULL);
        ESP_ERROR_CHECK(mhr_submit_succes_cell((uint32_t)tv.tv_sec, 1));
        ESP_LOGI(TAG, "SENT SUBMIT");
    }
    vTaskDelete(NULL);
}

void app_main(void) {
    ++boot_count;
    ESP_LOGI(TAG, "Boot count: %d", boot_count);

    main_flow();

    init_cells();

    xTaskCreate(&button_task, "submit-pills-task", 8192, NULL, 1, NULL);

    run_scheduler_task();

    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

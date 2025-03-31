#include "cell_activity_watchdog.h"
#include "button_controller.h"
#include "buzzer.h"
#include "cell_led_controller.h"
#include "display_error.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "medsenger_http_requests.h"
#include "medsenger_refresh_rate.h"
#include "medsenger_synced.h"
#include "portmacro.h"
#include "schedule_data.h"
#include "sdkconfig.h"
#include "send_event_data.h"
#include <stdint.h>
#include <sys/time.h>

static const char *TAG = "cell-activity-watchdog";

static SemaphoreHandle_t button_pressed;

static void is_button_pressed_listner(void *params) {
    assert(xSemaphoreTake(button_pressed, portMAX_DELAY) == pdTRUE);
    ESP_LOGI(TAG, "Started button listener...");
    bc_wait_for_single_press();
    xSemaphoreGive(button_pressed);
    vTaskDelete(NULL);
}

static esp_err_t cdc_process_cell(sd_cell_schedule_t cell, uint8_t indx) {
    if (!gm_get_medsenger_synced() &&
        !sd_get_processing_without_connection_allowed(&cell)) {
        de_display_error(ACTIVE_CELL_WITH_FAILED_CONNECTION);
        return ESP_FAIL;
    }

    cdc_enable_signal(indx);

    return ESP_OK;
}

esp_err_t cdc_monitor(void) {
    button_pressed = xSemaphoreCreateMutex();
    assert(button_pressed != NULL);

    xTaskCreate(&is_button_pressed_listner, "is-button-pressed-listener", 4096,
                NULL, 1, NULL);

    uint8_t i;
    esp_err_t err;
    bool active_cells[CONFIG_SD_CELLS_COUNT] = {0};
    sd_cell_schedule_t schedule[CONFIG_SD_CELLS_COUNT];
    for (i = 0; i < CONFIG_SD_CELLS_COUNT; ++i)
        schedule[i] = sd_get_schedule_by_cell_indx(i);

    while (1) {
        struct timeval tv;
        gettimeofday(&tv, NULL);

        bool play_signal = false;
        for (i = 0; i < CONFIG_SD_CELLS_COUNT; ++i) {

            /*ESP_LOGI(TAG, "checking cell is active: [%d], st: [%"PRIu32"], et: [%"PRIu32"], cur: [%lld]", */
            /*         active_cells[i], schedule[i].start_timestamp, schedule[i].end_timestamp, tv.tv_sec);*/

            if (!active_cells[i] &&
                (uint32_t)tv.tv_sec > schedule[i].start_timestamp &&
                (uint32_t)tv.tv_sec < schedule[i].end_timestamp) {

                ESP_LOGI(TAG, "Found active cell [%d] ACTIVATING", i);

                active_cells[i] = true;

                err = cdc_process_cell(schedule[i], i);
                if (err != ESP_OK) {
                    return err;
                }
                play_signal = true;
            } else if (active_cells[i] &&
                       (uint32_t)tv.tv_sec > schedule[i].end_timestamp) {

                ESP_LOGI(TAG, "Cell [%d] no longer active", i);

                cdc_disable_signal(i);

                active_cells[i] = false;
            }
        }

        if (play_signal)
            b_play_notification(PILL_NOTIFICATION);

        // check if none of cells is active
        bool is_true_in_array = false;
        for (i = 0; i < CONFIG_SD_CELLS_COUNT; i++)
            is_true_in_array = is_true_in_array ? true : active_cells[i];
        if (!is_true_in_array) {
            ESP_LOGI(TAG, "No tasks found. Exiting...");
            // NO tasks
            return ESP_OK;
        }

        if (gm_get_medsenger_refresh_rate_sec() <
            (esp_timer_get_time() / 1000000)) {
            ESP_LOGI(TAG, "Refresh rate limit reached. Restarting...");
            // restart process to fetch data
            esp_restart();
        }

        if (xSemaphoreTake(button_pressed, (TickType_t)10) == pdTRUE) {
            // button pressed
            ESP_LOGI(TAG, "Button pressed");

            for (i = 0; i < CONFIG_SD_CELLS_COUNT; ++i)
                if (active_cells[i]) {
                    cdc_disable_signal(i);
                    err = mhr_submit_succes_cell((uint32_t)tv.tv_sec, i);
                    if (err != ESP_OK) {
                        ESP_LOGI(TAG, "Failed to send to Medsenger");
                        se_send_event_t e = {
                            .timestamp = (uint32_t)tv.tv_sec,
                            .cell_indx = i,
                        };
                        ESP_ERROR_CHECK(se_add_fsent_event(e));
                    }
                }

            return ESP_OK;
        }

        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

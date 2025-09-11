#include "cell_activity_watchdog.h"
#include "button_controller.h"
#include "buzzer.h"
#include "cell_led_controller.h"
#include "cells_count.h"
#include "display_error.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "last_confirmed_cell.h"
#include "led_controller.h"
#include "medsenger_http_requests.h"
#include "medsenger_refresh_rate.h"
#include "medsenger_synced.h"
#include "portmacro.h"
#include "schedule_data.h"
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
        ESP_LOGE(TAG,
                 "Not synced with medsenger and processing without connection "
                 "not allowed. Cell meta: %" PRIu8,
                 cell.meta);
        display_error(DE_STAY_HOLDING);
        return ESP_FAIL;
    }

    cdc_enable_signal(indx);

    return ESP_OK;
}

static bool is_cell_was_already_submitted(sd_cell_schedule_t cell) {
    uint32_t times_start, times_end;
    ESP_ERROR_CHECK(load_last_confirmed_cell_intrv(&times_start, &times_end));
    return (cell.start_timestamp == times_start &&
            cell.end_timestamp == times_end);
}

esp_err_t cdc_monitor(const char *serial_nu) {
    button_pressed = xSemaphoreCreateMutex();
    assert(button_pressed != NULL);

    xTaskCreate(&is_button_pressed_listner, "is-button-pressed-listener", 4096,
                NULL, 1, NULL);

    uint8_t i;
    esp_err_t err;
    bool active_cells[CELLS_COUNT] = {0};
    sd_cell_schedule_t schedule[CELLS_COUNT];
    for (i = 0; i < CELLS_COUNT; ++i)
        schedule[i] = sd_get_schedule_by_cell_indx(i);

    uint32_t medsenger_refresh_rate;
    m_get_medsenger_refresh_rate_sec(&medsenger_refresh_rate);

    ESP_LOGI(TAG, "Got medsenger refresh rate: %" PRIu32,
             medsenger_refresh_rate);

    while (1) {
        struct timeval tv;
        gettimeofday(&tv, NULL);

        bool play_signal = false;
        for (i = 0; i < CELLS_COUNT; ++i) {

            // ESP_LOGI(TAG,
            //          "checking cell is active: [%d], st: [%" PRIu32
            //          "], et: [%" PRIu32 "], cur: [%lld]",
            //          active_cells[i], schedule[i].start_timestamp,
            //          schedule[i].end_timestamp, tv.tv_sec);

            if (!active_cells[i] &&
                (uint32_t)tv.tv_sec > schedule[i].start_timestamp &&
                (uint32_t)tv.tv_sec < schedule[i].end_timestamp &&
                !is_cell_was_already_submitted(schedule[i])) {

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
        for (i = 0; i < CELLS_COUNT; i++)
            is_true_in_array = is_true_in_array ? true : active_cells[i];
        if (!is_true_in_array) {
            ESP_LOGI(TAG,
                     "No tasks found for current timestamp: %" PRIu32
                     ". Exiting...",
                     (uint32_t)tv.tv_sec);
            // NO tasks
            return ESP_OK;
        }

        if (medsenger_refresh_rate < (esp_timer_get_time() / 1000000)) {
            ESP_LOGI(TAG, "Refresh rate limit reached. Restarting...");
            // restart process to fetch data
            esp_restart();
        }

        if (xSemaphoreTake(button_pressed, (TickType_t)10) == pdTRUE) {
            // button pressed
            ESP_LOGI(TAG, "Button pressed");
            vTaskDelay(500 / portTICK_PERIOD_MS);

            for (i = 0; i < CELLS_COUNT; ++i)
                if (active_cells[i]) {
                    cdc_disable_signal(i);
                    ESP_ERROR_CHECK(save_last_confirmed_cell_intrv(
                        schedule[i].start_timestamp,
                        schedule[i].end_timestamp));
                    err = mhr_submit_succes_cell((uint32_t)tv.tv_sec, i,
                                                 serial_nu);
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

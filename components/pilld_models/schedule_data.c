#include "schedule_data.h"
#include "cells_count.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "nvs.h"
#include "schedule_data_private.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define STORAGE_NAMESPACE "schd-data"
#define STORAGE_SCHEDULE_KEY "s"

const char *TAG = "cell-schedule-data";

static sd_cell_schedule_t sd_schedule_data[CELLS_COUNT];
static SemaphoreHandle_t sd_schedule_data_mutex;

void schedule_data_init(void) {
    sd_schedule_data_mutex = xSemaphoreCreateMutex();
    assert(sd_schedule_data_mutex != NULL);
}

sd_cell_schedule_t sd_get_schedule_by_cell_indx(int n) {
    assert(n < CELLS_COUNT);
    sd_cell_schedule_t buf;
    assert(xSemaphoreTake(sd_schedule_data_mutex, portMAX_DELAY) == pdTRUE);
    buf = sd_schedule_data[n];
    xSemaphoreGive(sd_schedule_data_mutex);
    return buf;
}

void sd_print_schedule(void) {
    for (int i = 0; i < CELLS_COUNT; i++) {
        sd_cell_schedule_t cs = sd_get_schedule_by_cell_indx(i);
        ESP_LOGI(TAG, "SC[%" PRIu32 ", %" PRIu32 ", %" PRIu8 "]",
                 cs.start_timestamp, cs.end_timestamp, cs.meta);
    }
}

esp_err_t sd_save_schedule(char *buf, unsigned int buf_length) {
    if ((buf_length / sizeof(sd_cell_schedule_t)) != CELLS_COUNT)
        return ESP_FAIL;
    esp_err_t err = sd_save_schedule_to_memory(buf, buf_length);
    if (err != ESP_OK) {
        return err;
    }
    return sd_save_to_flash(buf, buf_length);
}

esp_err_t sd_save_schedule_to_memory(void *buf, size_t buf_length) {
    if (xSemaphoreTake(sd_schedule_data_mutex, portMAX_DELAY) == pdTRUE) {
        memcpy(sd_schedule_data, buf, buf_length);
        xSemaphoreGive(sd_schedule_data_mutex);
        return ESP_OK;
    } else {
        return ESP_FAIL;
    }
}

esp_err_t sd_save_to_flash(char *buf, unsigned int buf_length) {
    nvs_handle_t sd_nvs_handle;
    esp_err_t err;
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &sd_nvs_handle);
    if (err != ESP_OK)
        return err;
    err = nvs_set_blob(sd_nvs_handle, STORAGE_SCHEDULE_KEY, buf, buf_length);
    if (err != ESP_OK)
        return err;
    err = nvs_commit(sd_nvs_handle);
    if (err != ESP_OK)
        return err;
    nvs_close(sd_nvs_handle);
    return ESP_OK;
}

esp_err_t sd_load_schedule_from_flash(void) {
    nvs_handle_t sd_nvs_handle;
    esp_err_t err;
    err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &sd_nvs_handle);
    if (err != ESP_OK)
        return err;
    size_t required_size = 0;
    err =
        nvs_get_blob(sd_nvs_handle, STORAGE_SCHEDULE_KEY, NULL, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
        return err;
    if (required_size == 0) {
        nvs_close(sd_nvs_handle);
        return ESP_FAIL;
    } else {
        assert(required_size == sizeof(sd_schedule_data));
        if (xSemaphoreTake(sd_schedule_data_mutex, portMAX_DELAY) == pdTRUE) {
            err = nvs_get_blob(sd_nvs_handle, STORAGE_SCHEDULE_KEY,
                               sd_schedule_data, &required_size);
            xSemaphoreGive(sd_schedule_data_mutex);
            if (err != ESP_OK)
                return err;
            nvs_close(sd_nvs_handle);
            return ESP_OK;
        } else {
            nvs_close(sd_nvs_handle);
            return ESP_FAIL;
        }
    }
}

bool sd_get_processing_without_connection_allowed(sd_cell_schedule_t *cell) {
    return cell->meta & (1 << 1);
}

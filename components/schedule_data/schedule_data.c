#include "schedule_data.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "nvs.h"
#include "schedule_data_private.h"
#include "sdkconfig.h"
#include <assert.h>
#include <string.h>

#define STORAGE_NAMESPACE "schedule-storage"
#define STORAGE_SCHEDULE_KEY "cell-schedule"

const char *TAG = "cell-schedule-data";

static sd_cell_schedule_t sd_schedule_data[CONFIG_SD_CELLS_COUNT];
static SemaphoreHandle_t sd_schedule_data_mutex;

void sd_init(void) {
    sd_schedule_data_mutex = xSemaphoreCreateMutex();
    if (sd_schedule_data_mutex == NULL) {
        abort();
    }
}

sd_cell_schedule_t sd_get_schedule_by_cell_indx(int n) {
    assert(n < CONFIG_SD_CELLS_COUNT);
    sd_cell_schedule_t buf;
    if (xSemaphoreTake(sd_schedule_data_mutex, portMAX_DELAY) == pdTRUE) {
        buf = sd_schedule_data[n];
        xSemaphoreGive(sd_schedule_data_mutex);
    } else {
        abort();
    }
    return buf;
}

void sd_print_schedule(void) {
    for (int i = 0; i < CONFIG_SD_CELLS_COUNT; i++) {
        sd_cell_schedule_t cs = sd_get_schedule_by_cell_indx(i);
        ESP_LOGI(TAG, "SC[%" PRIu32 ", %" PRIu32 ", %" PRIu8 "]",
                 cs.start_timestamp, cs.end_timestamp, cs.meta);
    }
}

esp_err_t sd_save_schedule(char *buf, unsigned int buf_length) {
    if ((buf_length / sizeof(sd_cell_schedule_t)) != CONFIG_SD_CELLS_COUNT)
        return ESP_FAIL;
    if (xSemaphoreTake(sd_schedule_data_mutex, portMAX_DELAY) == pdTRUE) {
        memcpy(sd_schedule_data, buf, buf_length);
        xSemaphoreGive(sd_schedule_data_mutex);
        return sd_save_to_flash();
    } else {
        return ESP_FAIL;
    }
}

esp_err_t sd_save_to_flash(void) {
    nvs_handle_t sd_nvs_handle;
    esp_err_t err;

    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &sd_nvs_handle);
    if (err != ESP_OK)
        return err;

    err = nvs_set_blob(sd_nvs_handle, STORAGE_SCHEDULE_KEY, sd_schedule_data,
                       sizeof(sd_schedule_data));
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

    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &sd_nvs_handle);
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
        err = nvs_get_blob(sd_nvs_handle, "run_time", sd_schedule_data,
                           &required_size);
        if (err != ESP_OK)
            return err;
        nvs_close(sd_nvs_handle);
        return ESP_OK;
    }
}

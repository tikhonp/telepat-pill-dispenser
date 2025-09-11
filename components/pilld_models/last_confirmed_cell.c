#include "last_confirmed_cell.h"
#include "nvs.h"

#define NAMESPACE "last-conf-cell"
#define KEY_START "start"
#define KEY_END "end"

esp_err_t save_last_confirmed_cell_intrv(uint32_t timestamp_start,
                                         uint32_t timestamp_end) {
    nvs_handle_t sd_nvs_handle;
    esp_err_t err;
    err = nvs_open(NAMESPACE, NVS_READWRITE, &sd_nvs_handle);
    if (err != ESP_OK)
        return err;

    err = nvs_set_u32(sd_nvs_handle, KEY_START, timestamp_start);
    if (err != ESP_OK)
        return err;

    err = nvs_set_u32(sd_nvs_handle, KEY_END, timestamp_end);
    if (err != ESP_OK)
        return err;

    err = nvs_commit(sd_nvs_handle);
    if (err != ESP_OK)
        return err;

    nvs_close(sd_nvs_handle);
    return ESP_OK;
}

esp_err_t load_last_confirmed_cell_intrv(uint32_t *timestamp_start,
                                         uint32_t *timestamp_end) {
    nvs_handle_t sd_nvs_handle;
    esp_err_t err;

    err = nvs_open(NAMESPACE, NVS_READONLY, &sd_nvs_handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        *timestamp_start = 0;
        *timestamp_end = 0;
        return ESP_OK;
    }
    if (err != ESP_OK)
        return err;

    err = nvs_get_u32(sd_nvs_handle, KEY_START, timestamp_start);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
        return err;

    err = nvs_get_u32(sd_nvs_handle, KEY_END, timestamp_end);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
        return err;

    nvs_close(sd_nvs_handle);
    return ESP_OK;
}

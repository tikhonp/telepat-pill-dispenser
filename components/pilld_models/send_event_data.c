#include "send_event_data.h"
#include "esp_log.h"
#include "nvs.h"

#define STORAGE_EVNTS_NAMESPACE "sevt-data"
#define STORAGE_EVNTS_KEY "e"

#define TAG "send-event-data"

esp_err_t se_add_fsent_event(se_send_event_t event) {
    nvs_handle_t sd_nvs_handle;
    esp_err_t err;
    err = nvs_open(STORAGE_EVNTS_NAMESPACE, NVS_READWRITE, &sd_nvs_handle);
    if (err != ESP_OK)
        return err;

    size_t required_size = 0;
    err = nvs_get_blob(sd_nvs_handle, STORAGE_EVNTS_KEY, NULL, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
        return err;
    ESP_LOGI(TAG, "Current sent events size: %d bytes, err: %s", required_size,
             esp_err_to_name(err));
    if (err == ESP_ERR_NVS_NOT_FOUND)
        required_size = 0;

    se_send_event_t *buf = malloc(sizeof(se_send_event_t) + required_size);

    if (required_size != 0) {
        err =
            nvs_get_blob(sd_nvs_handle, STORAGE_EVNTS_KEY, buf, &required_size);
        if (err != ESP_OK)
            return err;
    }
    buf[required_size / sizeof(se_send_event_t)] = event;

    err = nvs_set_blob(sd_nvs_handle, STORAGE_EVNTS_KEY, buf,
                       sizeof(se_send_event_t) + required_size);
    if (err != ESP_OK)
        return err;
    err = nvs_commit(sd_nvs_handle);
    if (err != ESP_OK)
        return err;
    nvs_close(sd_nvs_handle);
    return ESP_OK;
}

esp_err_t se_get_fsent_events(se_send_event_t *buf, size_t buf_size,
                              size_t *elements_count) {
    *elements_count = 0;
    nvs_handle_t sd_nvs_handle;
    esp_err_t err;
    err = nvs_open(STORAGE_EVNTS_NAMESPACE, NVS_READONLY, &sd_nvs_handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No sent events found");
        return ESP_OK;
    }
    if (err != ESP_OK)
        return err;
    size_t required_size = 0;
    err = nvs_get_blob(sd_nvs_handle, STORAGE_EVNTS_KEY, NULL, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
        return err;
    if (required_size == 0 || err == ESP_ERR_NVS_NOT_FOUND) {
        nvs_close(sd_nvs_handle);
        ESP_LOGI(TAG, "No sent events found");
        return ESP_OK;
    } else {
        ESP_LOGI(TAG, "Found %d sent events",
                 required_size / sizeof(se_send_event_t));
        *elements_count = required_size / sizeof(se_send_event_t);
        assert(required_size <= (sizeof(se_send_event_t) * buf_size));
        err =
            nvs_get_blob(sd_nvs_handle, STORAGE_EVNTS_KEY, buf, &required_size);
        if (err != ESP_OK)
            return err;
        nvs_close(sd_nvs_handle);
        return ESP_OK;
    }
}

esp_err_t se_save_fsent_events(se_send_event_t *buf, size_t events_count) {
    nvs_handle_t sd_nvs_handle;
    esp_err_t err;
    err = nvs_open(STORAGE_EVNTS_NAMESPACE, NVS_READWRITE, &sd_nvs_handle);
    if (err != ESP_OK)
        return err;
    err = nvs_set_blob(sd_nvs_handle, STORAGE_EVNTS_KEY, buf,
                       sizeof(se_send_event_t) * events_count);
    if (err != ESP_OK)
        return err;
    err = nvs_commit(sd_nvs_handle);
    if (err != ESP_OK)
        return err;
    nvs_close(sd_nvs_handle);
    return ESP_OK;
}

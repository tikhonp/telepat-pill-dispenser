#include "medsenger_refresh_rate.h"
#include "nvs.h"

#define STORAGE_NAMESPACE "mdsnr-data"
#define STORAGE_SCHEDULE_KEY "rr"

void m_get_medsenger_refresh_rate_sec(uint32_t *refresh_rate) {
    *refresh_rate = 3600;
    esp_err_t err;
    nvs_handle_t m_handle;

    err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &m_handle);
    if (err != ESP_OK)
        return;
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        *refresh_rate = 0;
        return;
    }

    err = nvs_get_u32(m_handle, STORAGE_SCHEDULE_KEY, refresh_rate);
    nvs_close(m_handle);
    return;
}

esp_err_t m_save_medsenger_refresh_rate_sec(uint32_t refresh_rate) {
    esp_err_t err;
    nvs_handle_t m_handle;

    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &m_handle);
    if (err != ESP_OK)
        return err;
    err = nvs_set_u32(m_handle, STORAGE_SCHEDULE_KEY, refresh_rate);
    if (err != ESP_OK) {
        nvs_close(m_handle);
        return err;
    }
    err = nvs_commit(m_handle);
    nvs_close(m_handle);
    return err;
}

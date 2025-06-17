#include "wifi_creds.h"
#include "esp_err.h"
#include "nvs.h"
#include <stddef.h>

#define STORAGE_WIFI_NAMESPACE "wifi-data"
#define STORAGE_WIFI_SSID_KEY "ssid"
#define STORAGE_WIFI_PSK_KEY "psk"

esp_err_t gm_get_wifi_creds(wifi_creds_t *creds) {
    size_t length;
    nvs_handle_t nvs_handle;
    esp_err_t err;
    err = nvs_open(STORAGE_WIFI_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK)
        return err;
    err = nvs_get_str(nvs_handle, STORAGE_WIFI_SSID_KEY, creds->ssid, &length);
    if (err != ESP_OK)
        return err;
    if (length == 0)
        return ESP_FAIL;

    err = nvs_get_str(nvs_handle, STORAGE_WIFI_PSK_KEY, creds->psk, &length);
    if (err != ESP_OK)
        return err;
    if (length == 0)
        return ESP_FAIL;

    return ESP_OK;
}

esp_err_t gm_set_wifi_creds(wifi_creds_t *creds) {
    nvs_handle_t nvs_handle;
    esp_err_t err;
    err = nvs_open(STORAGE_WIFI_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
        return err;
    if (creds == NULL) {
        err = nvs_erase_key(nvs_handle, STORAGE_WIFI_SSID_KEY);
        if (err != ESP_OK)
            return err;
        err = nvs_erase_key(nvs_handle, STORAGE_WIFI_PSK_KEY);
        if (err != ESP_OK)
            return err;
    } else {
        err = nvs_set_str(nvs_handle, STORAGE_WIFI_SSID_KEY, creds->ssid);
        if (err != ESP_OK)
            return err;
        err = nvs_set_str(nvs_handle, STORAGE_WIFI_PSK_KEY, creds->psk);
        if (err != ESP_OK)
            return err;
    }
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK)
        return err;
    nvs_close(nvs_handle);
    return ESP_OK;
}

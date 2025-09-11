#include "mfg_data.h"
#include "esp_err.h"
#include "nvs_flash.h"

#define TAG "mfg-data"
#define PARTITION_NAME "nvs_mfg"
#define NAMESPACE "mfg"

#define DEFAULT_SSID "DEFAULT_SSID"
#define DEFAULT_PSK "DEFAULT_PSK"
#define SERIAL_NU "SERIAL_NU"

esp_err_t mfg_data_init(void) {
    return nvs_flash_init_partition(PARTITION_NAME);
}

esp_err_t read_serial_nu(char *serial_nu, size_t buf_len) {
    esp_err_t err;
    nvs_handle_t h;
    err = nvs_open_from_partition(PARTITION_NAME, NAMESPACE, NVS_READONLY, &h);
    if (err != ESP_OK) {
        return err;
    }

    size_t required = 0;
    err = nvs_get_str(h, SERIAL_NU, NULL, &required);
    if (err != ESP_OK) {
        nvs_close(h);
        return err;
    } else if (required > buf_len) {
        nvs_close(h);
        return ESP_ERR_NVS_INVALID_LENGTH;
    } else if (required == 0) {
        nvs_close(h);
        return ESP_ERR_NVS_NOT_FOUND;
    } else {
        err = nvs_get_str(h, SERIAL_NU, serial_nu, &required);
        nvs_close(h);
        return err;
    }
}

esp_err_t read_default_wifi_creds(char *ssid, size_t ssid_buf_len, char *psk,
                                  size_t psk_buf_len) {
    esp_err_t err;
    nvs_handle_t h;
    err = nvs_open_from_partition(PARTITION_NAME, NAMESPACE, NVS_READONLY, &h);
    if (err != ESP_OK) {
        return err;
    }

    size_t required_ssid = 0;
    err = nvs_get_str(h, DEFAULT_SSID, NULL, &required_ssid);
    if (err != ESP_OK) {
        nvs_close(h);
        return err;
    } else if (required_ssid > ssid_buf_len) {
        nvs_close(h);
        return ESP_ERR_NVS_INVALID_LENGTH;
    } else if (required_ssid == 0) {
        nvs_close(h);
        return ESP_ERR_NVS_NOT_FOUND;
    } else {
        err = nvs_get_str(h, DEFAULT_SSID, ssid, &required_ssid);
        if (err != ESP_OK) {
            nvs_close(h);
            return err;
        }
    }

    size_t required_psk = 0;
    err = nvs_get_str(h, DEFAULT_PSK, NULL, &required_psk);
    if (err != ESP_OK) {
        nvs_close(h);
        return err;
    } else if (required_psk > psk_buf_len) {
        nvs_close(h);
        return ESP_ERR_NVS_INVALID_LENGTH;
    } else if (required_psk == 0) {
        nvs_close(h);
        return ESP_ERR_NVS_NOT_FOUND;
    } else {
        err = nvs_get_str(h, DEFAULT_PSK, psk, &required_psk);
        nvs_close(h);
        return err;
    }
}

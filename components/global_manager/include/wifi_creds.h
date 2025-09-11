#pragma once

#include "esp_err.h"

#define SSID_BUF_LEN 33
#define PSK_BUF_LEN 65

typedef struct {
    char ssid[SSID_BUF_LEN];
    char psk[PSK_BUF_LEN];
} wifi_creds_t;

/*
Get WiFi credentials from persistent storage
If credentials are not set, return ESP_ERR_NOT_FOUND.
*/
esp_err_t gm_get_wifi_creds(wifi_creds_t *creds);

/*
Set WiFi credentials to persistent storage
If credentials are NULL, remove existing credentials.
*/
esp_err_t gm_set_wifi_creds(wifi_creds_t *creds);

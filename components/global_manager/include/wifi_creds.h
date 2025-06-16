#pragma once

#include "esp_err.h"

typedef struct {
    char ssid[33];
    char psk[65];
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

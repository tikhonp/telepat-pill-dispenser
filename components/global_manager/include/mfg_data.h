#pragma once

#include "esp_err.h"
#include <stddef.h>

#define SERIAL_NU_BUF_LEN 10

esp_err_t mfg_data_init(void);
esp_err_t read_serial_nu(char *serial_nu, size_t buf_len);
esp_err_t read_default_wifi_creds(char *ssid, size_t ssid_buf_len, char *psk,
                                  size_t psk_buf_len);

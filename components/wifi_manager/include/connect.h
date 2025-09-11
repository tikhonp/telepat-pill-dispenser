#pragma once

#include "esp_err.h"

/*
Registers Wi-Fi stack and connects it to network
*/
esp_err_t wm_connect(const char *serial_nu);

/*
Disabled connection and turns off the Wi-Fi
*/
esp_err_t wm_disconnect(void);

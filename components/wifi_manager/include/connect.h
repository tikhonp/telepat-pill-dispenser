#pragma once

#include "esp_err.h"

/*
Registers Wi-Fi stack and connects it to network
*/
esp_err_t connect(void);

/*
Disabled connection and turns off the Wi-Fi
*/
esp_err_t disconnect(void);

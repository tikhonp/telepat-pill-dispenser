#pragma once

#include "esp_err.h"

enum b_notification_t {
    FATAL_ERROR,
    PILL_NOTIFICATION,
};

void b_init(void);

/*
Tells buzzer thread to play specific notification pattern.
*/
esp_err_t b_play_notification(enum b_notification_t notification);

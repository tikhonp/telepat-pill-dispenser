#pragma once

typedef enum {
    DE_RED = 100,
    DE_STAY_HOLDING = 101,
    DE_GREEN = 102,
    DE_OK = 103,
    DE_WIFI_CONNECTED = 104,
    DE_SYNC_FAILED = 105,
    DE_WIFI = 106,
    DE_YELLOW = 107,
} de_error_code_t;

void de_start_blinking(de_error_code_t error_code);
void de_stop_blinking(void);

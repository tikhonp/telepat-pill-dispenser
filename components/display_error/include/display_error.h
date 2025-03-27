#pragma once

typedef enum {
    WIFI_FAILED,
    FLASH_LOAD_FAILED,
    ACTIVE_CELL_WITH_FAILED_CONNECTION,
} de_fatal_error_t;

void de_init(void);
void de_display_error(de_fatal_error_t error);

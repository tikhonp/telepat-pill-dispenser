#pragma once

typedef enum {
    WIFI_FAILED,
    FLASH_LOAD_FAILED,
} de_fatal_error_t;

void display_error(de_fatal_error_t error);

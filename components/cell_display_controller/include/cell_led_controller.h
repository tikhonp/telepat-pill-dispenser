#pragma once

#include <stdint.h>

static const uint8_t cdc_cell_indx_to_gpio_map[] = {
    1,
    2,
    3,
    4,
};

void cdc_init_led_signals(void);
void cdc_enable_signal(uint8_t indx);
void cdc_disable_signal(uint8_t indx);

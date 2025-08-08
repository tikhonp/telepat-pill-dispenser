#pragma once

#include <stdint.h>

void cdc_init_led_signals(void);
void cdc_enable_signal(uint8_t indx);
void cdc_disable_signal(uint8_t indx);
void cdc_deinit_led_signals(void);
// Set a specific RGB color on a single LED immediately
void cdc_set_signal_color(uint8_t indx, uint8_t r, uint8_t g, uint8_t b);

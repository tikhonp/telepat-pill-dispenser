#pragma once

#include <stdint.h>

void cdc_init_led_signals(void);
void cdc_enable_signal(uint8_t indx);
void cdc_disable_signal(uint8_t indx);
void cdc_deinit_led_signals(void);

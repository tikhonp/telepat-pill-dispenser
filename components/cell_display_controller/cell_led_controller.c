#include "cell_led_controller.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

void cdc_init_led_signals(void) {
    for (int i = 0; i < 4; i++) {
        gpio_reset_pin(cdc_cell_indx_to_gpio_map[i]);
        gpio_set_direction(cdc_cell_indx_to_gpio_map[i], GPIO_MODE_OUTPUT);
    }
}

void cdc_enable_signal(uint8_t indx) {
    assert(indx < CONFIG_SD_CELLS_COUNT);
    gpio_set_level(cdc_cell_indx_to_gpio_map[indx], 1);
}

void cdc_disable_signal(uint8_t indx) {
    assert(indx < CONFIG_SD_CELLS_COUNT);
    gpio_set_level(cdc_cell_indx_to_gpio_map[indx], 0);
}

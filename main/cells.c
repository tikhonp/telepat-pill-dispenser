#include "buzzer.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: export
#include <stdint.h>

#define ZERO_CELL_PIN 1

static const char *TAG = "CELLS";

void init_cells() {
    for (int i = 0; i < 4; i++) {
        gpio_reset_pin(ZERO_CELL_PIN + i);
        gpio_set_direction(ZERO_CELL_PIN + i, GPIO_MODE_OUTPUT);
    }
}

void enable_cell(int indx) {
    if (indx >= 4) {
        ESP_LOGE(TAG, "Invalid cell indx");
        abort();
    }
    gpio_set_level(ZERO_CELL_PIN + indx, 1);
    b_play_notification(PILL_NOTIFICATION);
}

void disable_cell(int indx) {
    if (indx >= 4) {
        ESP_LOGE(TAG, "Invalid cell indx");
        abort();
    }
    gpio_set_level(ZERO_CELL_PIN + indx, 0);
}

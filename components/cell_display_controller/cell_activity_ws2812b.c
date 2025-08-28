#include "cell_led_controller.h"
#include "cells_count.h"
#include "ws2812b_controller.h"
#include <driver/gpio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <led_strip.h>
#include <stdint.h>
#include <string.h>

static const char *TAG = "leds-ws2812-controller";

#define LED_GPIO CONFIG_WS2812B_CONTROL_GPIO
#define POWER_GPIO CONFIG_WS2812B_ENABLE_GPIO

static SemaphoreHandle_t leds_mu;
static bool leds_state[CELLS_COUNT] = {0};

static void update_leds_strip(void) {
    for (uint8_t i = 0; i < CELLS_COUNT; i++) {
        if (leds_state[i]) {
            ws2812b_set_pixel(i, 255, 0, 0); // красный
        } else {
            ws2812b_set_pixel(i, 0, 0, 0);
        }
    }
    ws2812b_refresh();
}

void cdc_init_led_signals(void) {
    leds_mu = xSemaphoreCreateMutex();
    assert(leds_mu != NULL);

    // Включаем питание на POWER_GPIO
    gpio_reset_pin(POWER_GPIO);
    gpio_set_direction(POWER_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(POWER_GPIO, 1);
    ESP_LOGI(TAG, "POWER GPIO %d set HIGH", POWER_GPIO);

    if (!ws2812b_is_initialized()) {
        if (!ws2812b_init(LED_GPIO, CELLS_COUNT)) {
            ESP_LOGE(TAG, "Failed to initialize WS2812B strip");
            return;
        }
    }

    ESP_LOGI(TAG, "Using WS2812B strip on GPIO %d with %d LEDs", LED_GPIO,
             CELLS_COUNT);

    memset(leds_state, 0, sizeof(leds_state));
    update_leds_strip();
}

void cdc_enable_signal(uint8_t indx) {
    if (indx >= CELLS_COUNT) {
        ESP_LOGW(TAG, "Invalid LED index: %d", indx);
        return;
    }

    xSemaphoreTake(leds_mu, portMAX_DELAY);
    leds_state[indx] = true;
    update_leds_strip();
    xSemaphoreGive(leds_mu);
}

void cdc_disable_signal(uint8_t indx) {
    if (indx >= CELLS_COUNT) {
        ESP_LOGW(TAG, "Invalid LED index: %d", indx);
        return;
    }

    xSemaphoreTake(leds_mu, portMAX_DELAY);
    leds_state[indx] = false;
    update_leds_strip();
    xSemaphoreGive(leds_mu);
}

void cdc_deinit_led_signals(void) {
    if (leds_mu == NULL) {
        ESP_LOGW(TAG, "LED semaphore not initialized, skipping deinit");
        return;
    }
    xSemaphoreTake(leds_mu, portMAX_DELAY);
    memset(leds_state, 0, sizeof(leds_state));
    update_leds_strip();
    xSemaphoreGive(leds_mu);

    // Удаляем только мьютекс, лента не деинициализируется, так как может использоваться другими компонентами
    vSemaphoreDelete(leds_mu);
    leds_mu = NULL;
    // Выключаем питание на POWER_GPIO
    gpio_set_level(POWER_GPIO, 0);
    ESP_LOGI(TAG, "POWER GPIO %d set LOW", POWER_GPIO);

    ESP_LOGI(TAG, "WS2812B controller usage ended");
}

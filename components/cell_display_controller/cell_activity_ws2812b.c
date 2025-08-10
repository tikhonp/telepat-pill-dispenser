#include "cell_led_controller.h"
#include "sdkconfig.h"
#include <esp_err.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <led_strip.h>
#include <stdint.h>
#include <string.h>

static const char *TAG = "leds-ws2812-controller";

#define LED_GPIO CONFIG_WS2812B_GPIO
#define LED_COUNT CONFIG_SD_CELLS_COUNT
#define LED_RES_HZ (10 * 1000 * 1000) // 10MHz

static led_strip_handle_t led_strip = NULL;
static SemaphoreHandle_t leds_mu;
static bool leds_state[LED_COUNT] = {0};
static bool leds_initialized = false;

static void update_leds_strip(void) {
    if (!leds_initialized || led_strip == NULL)
        return;
    for (uint8_t i = 0; i < LED_COUNT; i++) {
        if (leds_state[i]) {
            led_strip_set_pixel(led_strip, i, 255, 0, 0); // красный
        } else {
            led_strip_set_pixel(led_strip, i, 0, 0, 0);
        }
    }
    led_strip_refresh(led_strip);
}

void cdc_init_led_signals(void) {
    leds_mu = xSemaphoreCreateMutex();
    assert(leds_mu != NULL);

    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_GPIO,
        .max_leds = LED_COUNT,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB,
        .led_model = LED_MODEL_WS2812,
        .flags.invert_out = false,
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = LED_RES_HZ,
        .mem_block_symbols = 0,
    };

    ESP_LOGI(TAG, "Initializing WS2812B strip on GPIO %d with %d LEDs",
             LED_GPIO, LED_COUNT);
    esp_err_t ret =
        led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);
    if (ret != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Failed to create RMT TX channel (error %d). LED signals disabled",
            ret);
        return;
    }
    leds_initialized = true;

    memset(leds_state, 0, sizeof(leds_state));
    update_leds_strip();
}

void cdc_enable_signal(uint8_t indx) {
    if (!leds_initialized || led_strip == NULL) {
        ESP_LOGW(TAG, "LEDs not initialized, cannot enable signal");
        return;
    }
    if (indx >= LED_COUNT) {
        ESP_LOGW(TAG, "Invalid LED index: %d", indx);
        return;
    }

    xSemaphoreTake(leds_mu, portMAX_DELAY);
    leds_state[indx] = true;
    update_leds_strip();
    xSemaphoreGive(leds_mu);
}

void cdc_disable_signal(uint8_t indx) {
    if (!leds_initialized || led_strip == NULL) {
        ESP_LOGW(TAG, "LEDs not initialized, cannot disable signal");
        return;
    }
    if (indx >= LED_COUNT) {
        ESP_LOGW(TAG, "Invalid LED index: %d", indx);
        return;
    }

    xSemaphoreTake(leds_mu, portMAX_DELAY);
    leds_state[indx] = false;
    update_leds_strip();
    xSemaphoreGive(leds_mu);
}

void cdc_deinit_led_signals(void) {
    if (!leds_initialized || led_strip == NULL)
        return;
    xSemaphoreTake(leds_mu, portMAX_DELAY);
    memset(leds_state, 0, sizeof(leds_state));
    update_leds_strip();
    xSemaphoreGive(leds_mu);
    ESP_LOGI(TAG, "WS2812B deinit complete");
    // Delete LED strip driver and free resources
    led_strip_del(led_strip);
    led_strip = NULL;
    leds_initialized = false;
    vSemaphoreDelete(leds_mu);
}

// Set a specific RGB color on a single LED immediately
void cdc_set_signal_color(uint8_t indx, uint8_t r, uint8_t g, uint8_t b) {
    if (!leds_initialized || led_strip == NULL) {
        ESP_LOGW(TAG, "LEDs not initialized, cannot set color");
        return;
    }
    if (indx >= LED_COUNT) {
        ESP_LOGW(TAG, "Invalid LED index: %d", indx);
        return;
    }
    xSemaphoreTake(leds_mu, portMAX_DELAY);
    led_strip_set_pixel(led_strip, indx, r, g, b);
    led_strip_refresh(led_strip);
    xSemaphoreGive(leds_mu);
}

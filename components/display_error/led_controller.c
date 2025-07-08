#include "esp_err.h"
#include "led_indicator.h"
#include "led_strip_types.h"

#define LED_GPIO_NUM 10

enum {
    BLINK_TRIPLE = 0,
    BLINK_MAX,
};

static const blink_step_t triple_blink[] = {
    {LED_BLINK_HOLD, LED_STATE_OFF, 0},
    {LED_BLINK_BREATHE, LED_STATE_50_PERCENT, 500},
    {LED_BLINK_BREATHE, LED_STATE_OFF, 500},
    {LED_BLINK_LOOP, 0, 0},
};

blink_step_t const *led_mode[] = {
    [BLINK_TRIPLE] = triple_blink,
    [BLINK_MAX] = NULL,
};

void de_start_blinking(void) {
    led_indicator_strips_config_t strips_config = {
        .led_strip_cfg = {
            .strip_gpio_num = LED_GPIO_NUM,
            .max_leds = 1,
            .led_pixel_format = LED_PIXEL_FORMAT_GRB,
            .led_model = LED_MODEL_WS2812,
        }};

    const led_indicator_config_t config = {
        .mode = LED_STRIPS_MODE,
        .led_indicator_strips_config = &strips_config,
        .blink_lists = led_mode,
        .blink_list_num = BLINK_MAX,
    };

    led_indicator_handle_t led_handle = led_indicator_create(&config);
    assert(led_handle != NULL);

    ESP_ERROR_CHECK(led_indicator_start(led_handle, 0));
}

void de_stop_blinking(void) {
    led_indicator_strips_config_t strips_config = {
        .led_strip_cfg = {
            .strip_gpio_num = LED_GPIO_NUM,
            .max_leds = 1,
            .led_pixel_format = LED_PIXEL_FORMAT_GRB,
            .led_model = LED_MODEL_WS2812,
        }};

    const led_indicator_config_t config = {
        .mode = LED_STRIPS_MODE,
        .led_indicator_strips_config = &strips_config,
        .blink_lists = led_mode,
        .blink_list_num = BLINK_MAX,
    };

    led_indicator_handle_t led_handle = led_indicator_create(&config);
    ESP_ERROR_CHECK(led_indicator_set_on_off(led_handle, false));
}

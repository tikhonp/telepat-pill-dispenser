#include "led_indicator.h"
#include "led_strip_types.h"

#ifdef CONFIG_CDC_LEDS_WS2812B

#include "sdkconfig.h"
#include <led_strip.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <string.h>
#include <stdbool.h>

#define LED_GPIO_NUM CONFIG_WS2812B_GPIO
#define LED_COUNT CONFIG_SD_CELLS_COUNT
#define LED_RES_HZ (10 * 1000 * 1000) // 10 MHz

#define TAG "led-blinker"

typedef struct {
    uint8_t r, g, b;
    uint32_t duration_ms;
    bool smooth; // true - плавно, false - мгновенно
} BlinkStep;

typedef struct {
    const BlinkStep *steps;
    size_t step_count;
} BlinkPattern;

static led_strip_handle_t led_strip = NULL;
static TaskHandle_t blink_task_handle = NULL;
static int current_error_code = 0;

static const BlinkStep red_steps[] = {
    {255, 0, 0, 300, false},
    {0, 0, 0, 200, false},
};
static const BlinkPattern red_pattern = {
    .steps = red_steps,
    .step_count = sizeof(red_steps) / sizeof(BlinkStep),
};

static const BlinkStep stay_holding_steps[] = {
    {128, 0, 128, 150, false},
    {0, 0, 0, 150, false},
};
static const BlinkPattern stay_holding_pattern = {
    .steps = stay_holding_steps,
    .step_count = sizeof(stay_holding_steps) / sizeof(BlinkStep),
};

static const BlinkStep reset_steps[] = {
    {255, 0, 0, 150, false},
    {0, 0, 0, 150, false},
};
static const BlinkPattern reset_pattern = {
    .steps = reset_steps,
    .step_count = sizeof(reset_steps) / sizeof(BlinkStep),
};

static const BlinkStep OK_steps[] = {
    {0, 255, 0, 550, true},
    {0, 0, 0, 550, true},
};
static const BlinkPattern OK_pattern = {
    .steps = OK_steps,
    .step_count = sizeof(OK_steps) / sizeof(BlinkStep),
};

static const BlinkStep loading_steps[] = {
    {226, 232, 46, 350, false},
    {0, 0, 0, 350, false},
};
static const BlinkPattern loading_pattern = {
    .steps = loading_steps,
    .step_count = sizeof(loading_steps) / sizeof(BlinkStep),
};

static const BlinkStep waiting_steps[] = {
    {46, 232, 232, 1000, true},
    {0, 0, 0, 1000, true},
};
static const BlinkPattern waiting_pattern = {
    .steps = waiting_steps,
    .step_count = sizeof(waiting_steps) / sizeof(BlinkStep),
};

static const BlinkStep connected_steps[] = {
    {0, 232, 0, 1000, true},
    {0, 0, 0, 1000, true},
};
static const BlinkPattern connected_pattern = {
    .steps = connected_steps,
    .step_count = sizeof(connected_steps) / sizeof(BlinkStep),
};


// Инициализация ленты (один раз)
static void init_led_strip_once(void) {
    static bool initialized = false;
    static bool init_attempted = false;
    if (init_attempted) return;
    init_attempted = true;

    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_GPIO_NUM,
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

    esp_err_t ret = led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WS2812B init failed (error %d), LED indicator disabled", ret);
        return;
    }
    initialized = true;
}

// Плавное изменение цвета
static void led_smooth_transition(uint8_t r0, uint8_t g0, uint8_t b0,
                                  uint8_t r1, uint8_t g1, uint8_t b1,
                                  uint32_t duration_ms) {
    const uint32_t steps = duration_ms / 20;
    if (steps == 0) {
        for (int j = 0; j < LED_COUNT; ++j) {
            led_strip_set_pixel(led_strip, j, g1, r1, b1);
        }
        led_strip_refresh(led_strip);
        vTaskDelay(pdMS_TO_TICKS(duration_ms));
        return;
    }

    int r_diff = (int)r1 - (int)r0;
    int g_diff = (int)g1 - (int)g0;
    int b_diff = (int)b1 - (int)b0;

    for (uint32_t s = 1; s <= steps; ++s) {
        uint8_t r = (uint8_t)((int)r0 + (r_diff * (int)s) / (int)steps);
        uint8_t g = (uint8_t)((int)g0 + (g_diff * (int)s) / (int)steps);
        uint8_t b = (uint8_t)((int)b0 + (b_diff * (int)s) / (int)steps);

        for (int j = 0; j < LED_COUNT; ++j) {
            led_strip_set_pixel(led_strip, j, g, r, b);
        }
        led_strip_refresh(led_strip);
        vTaskDelay(pdMS_TO_TICKS(duration_ms / steps));
    }
}


// Задача мигания (паттерн повторяется по кругу)
static void led_blink_pattern_task(void *param) {
    const BlinkPattern *pattern = (const BlinkPattern *)param;
    uint8_t prev_r = 0, prev_g = 0, prev_b = 0;
    bool first = true;
    while (1) {
        for (size_t i = 0; i < pattern->step_count; ++i) {
            const BlinkStep *step = &pattern->steps[i];
            if (step->smooth && !first) {
                led_smooth_transition(prev_r, prev_g, prev_b, step->r, step->g, step->b, step->duration_ms);
            } else {
                for (int j = 0; j < LED_COUNT; ++j) {
                    led_strip_set_pixel(led_strip, j, step->r, step->g, step->b);
                }
                led_strip_refresh(led_strip);
                vTaskDelay(pdMS_TO_TICKS(step->duration_ms));
            }
            prev_r = step->r;
            prev_g = step->g;
            prev_b = step->b;
            first = false;
        }
    }
}

// Старт мигания с выбором паттерна по error_code
void de_start_blinking(int error_code) {
    init_led_strip_once();

    // Если уже запущена задача и error_code совпадает, ничего не делаем
    if (blink_task_handle != NULL) {
        if (current_error_code == error_code) {
            ESP_LOGW(TAG, "Blink task already running with same error_code");
            return;
        }
        // Остановить текущую задачу, если error_code другой
        vTaskDelete(blink_task_handle);
        blink_task_handle = NULL;
        // Выключить все светодиоды
        for (int j = 0; j < LED_COUNT; ++j) {
            led_strip_set_pixel(led_strip, j, 0, 0, 0);
        }
        led_strip_refresh(led_strip);
    }

    ESP_LOGI(TAG, "Starting blink task");

    const BlinkPattern *pattern = &red_pattern; // default
    switch (error_code) {
        case 101: // Example error code for red
            pattern = &stay_holding_pattern;
            break;
        case 102:
            pattern = &reset_pattern;
            break;
        case 103:
            pattern = &OK_pattern;
            break;
        case 104:
            pattern = &loading_pattern;
            break;
        case 105:
            pattern = &waiting_pattern;
            break;
        case 106:
            pattern = &connected_pattern;
            break;
        default:
            pattern = &red_pattern;
            break;
    }

    current_error_code = error_code;
    xTaskCreate(led_blink_pattern_task, "led_blink_pattern", 2048, (void *)pattern, 5, &blink_task_handle);
}

void de_stop_blinking(void) {
    if (blink_task_handle != NULL) {
        vTaskDelete(blink_task_handle);
        blink_task_handle = NULL;
        current_error_code = 0;
        // Выключить все светодиоды
        for (int j = 0; j < LED_COUNT; ++j) {
            led_strip_set_pixel(led_strip, j, 0, 0, 0);
        }
        led_strip_refresh(led_strip);
    }
}

#else

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

static led_indicator_handle_t led_handle = NULL;

void de_start_blinking(int error_code) {
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

    led_handle = led_indicator_create(&config);
    assert(led_handle != NULL);

    led_indicator_start(led_handle, 0);
}

void de_stop_blinking(void) {
    if (led_handle != NULL) {
        led_indicator_stop(led_handle, 0);
        led_indicator_delete(led_handle);
        led_handle = NULL;
    }
}

#endif

#include "led_indicator.h"
#include "led_strip_types.h"

#include "sdkconfig.h"
#include "ws2812b_controller.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <string.h>

#define LED_GPIO_NUM CONFIG_WS2812B_GPIO
#define LED_COUNT CONFIG_SD_CELLS_COUNT

#define TAG "led-blinker"

typedef struct {
    uint8_t r, g, b;
    uint32_t duration_ms;
    bool smooth; // new field: true for smooth transition
} BlinkStep;

typedef struct {
    const BlinkStep *steps;
    size_t step_count;
} BlinkPattern;

static TaskHandle_t blink_task_handle = NULL;

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

static const BlinkStep green_steps[] = {
    {0, 255, 0, 150, false},
    {0, 0, 0, 150, false},
};
static const BlinkPattern green_pattern = {
    .steps = green_steps,
    .step_count = sizeof(green_steps) / sizeof(BlinkStep),
};

static const BlinkStep wifi_connected_steps[] = {
    {0, 255, 0, 300, false},
    {0, 0, 0, 300, false},
};
static const BlinkPattern wifi_connected_pattern = {
    .steps = wifi_connected_steps,
    .step_count = sizeof(wifi_connected_steps) / sizeof(BlinkStep),
};

static const BlinkStep ok_steps[] = {
    {0, 255, 0, 2000, false},
};
static const BlinkPattern ok_pattern = {
    .steps = ok_steps,
    .step_count = sizeof(ok_steps) / sizeof(BlinkStep),
};

static const BlinkStep sync_failed_steps[] = {
    {255, 64, 255, 500, false},
    {0, 0, 0, 500, false},
};
static const BlinkPattern sync_failed_pattern = {
    .steps = sync_failed_steps,
    .step_count = sizeof(sync_failed_steps) / sizeof(BlinkStep),
};

static const BlinkStep wifi_steps[] = {
    {50, 27, 26, 1000, true},   // smooth transition to brownish
    {0, 0, 0, 1000, true},      // smooth transition to off
};
static const BlinkPattern wifi_pattern = {
    .steps = wifi_steps,
    .step_count = sizeof(wifi_steps) / sizeof(BlinkStep),
};

// Инициализация ленты (один раз)
static void init_led_strip_once(void) {
    if (!ws2812b_is_initialized()) {
        if (!ws2812b_init(LED_GPIO_NUM, LED_COUNT)) {
            ESP_LOGE(TAG, "Failed to initialize WS2812B strip");
        }
    }
}

// Задача мигания (паттерн повторяется по кругу)
static void led_blink_pattern_task(void *param) {
    const BlinkPattern *pattern = (const BlinkPattern *)param;
    size_t i = 0;
    while (true) {
        const BlinkStep *curr = &pattern->steps[i];
        const BlinkStep *next = &pattern->steps[(i + 1) % pattern->step_count];
        if (curr->smooth) {
            // Smooth transition from curr to next
            uint32_t steps = curr->duration_ms / 20; // 20ms per frame
            for (uint32_t s = 0; s < steps; ++s) {
                float t = (float)s / (float)steps;
                uint8_t r = curr->r + (int)((next->r - curr->r) * t);
                uint8_t g = curr->g + (int)((next->g - curr->g) * t);
                uint8_t b = curr->b + (int)((next->b - curr->b) * t);
                ws2812b_set_all_pixels(r, g, b);
                ws2812b_refresh();
                vTaskDelay(pdMS_TO_TICKS(20));
            }
        } else {
            ws2812b_set_all_pixels(curr->r, curr->g, curr->b);
            ws2812b_refresh();
            vTaskDelay(pdMS_TO_TICKS(curr->duration_ms));
        }
        i = (i + 1) % pattern->step_count;
    }
}

// Старт мигания с выбором паттерна по error_code
void de_start_blinking(int error_code) {
    init_led_strip_once();

    if (blink_task_handle != NULL) {
        ESP_LOGW(TAG, "Blink task already running");
        return;
    }

    ESP_LOGI(TAG, "Starting blink task");

    const BlinkPattern *pattern = &red_pattern; // default
    switch (error_code) {
        case 101: // Example error code for red
            pattern = &stay_holding_pattern;
            break;
        case 102:
            pattern = &green_pattern;
            break;
        case 103:
            pattern = &ok_pattern;
            break;
        case 104:
            pattern = &wifi_connected_pattern;
            break;
        case 105:
            pattern = &sync_failed_pattern;
            break;
        case 106:
            pattern = &wifi_pattern;
            break;
        default:
            pattern = &red_pattern;
            break;
    }

    xTaskCreate(led_blink_pattern_task, "led_blink_pattern", 2048, (void *)pattern, 5, &blink_task_handle);
}

void de_stop_blinking(void) {
    if (blink_task_handle != NULL) {
        vTaskDelete(blink_task_handle);
        blink_task_handle = NULL;
        // Выключить все светодиоды
        ws2812b_clear_all();
    }
}

// Функция деинициализации ленты
void de_led_strip_deinit(void) {
    de_stop_blinking();
    // Ленту не деинициализируем, так как она может использоваться другими компонентами
    // Очистка ленты уже произведена в de_stop_blinking
}

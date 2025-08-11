#include "led_indicator.h"
#include "led_strip_types.h"

#ifdef CONFIG_CDC_LEDS_WS2812B

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
} BlinkStep;

typedef struct {
    const BlinkStep *steps;
    size_t step_count;
} BlinkPattern;

static TaskHandle_t blink_task_handle = NULL;

static const BlinkStep red_steps[] = {
    {255, 0, 0, 300},
    {0, 0, 0, 200},
};
static const BlinkPattern red_pattern = {
    .steps = red_steps,
    .step_count = sizeof(red_steps) / sizeof(BlinkStep),
};

static const BlinkStep stay_holding_steps[] = {
    {128, 0, 128, 150},
    {0, 0, 0, 150},
};
static const BlinkPattern stay_holding_pattern = {
    .steps = stay_holding_steps,
    .step_count = sizeof(stay_holding_steps) / sizeof(BlinkStep),
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
    while (true) {
        for (size_t i = 0; i < pattern->step_count; ++i) {
            const BlinkStep *step = &pattern->steps[i];
            ws2812b_set_all_pixels(step->r, step->g, step->b);
            ws2812b_refresh();
            vTaskDelay(pdMS_TO_TICKS(step->duration_ms));
        }
        // После завершения паттерна цикл повторяется (по кругу)
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

// Функция деинициализации для альтернативного режима
void de_led_strip_deinit(void) {
    de_stop_blinking();
}

#endif

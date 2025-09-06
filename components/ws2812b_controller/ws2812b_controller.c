#include "ws2812b_controller.h"
#include <led_strip.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <string.h>
#include "driver/gpio.h"  // Add GPIO driver
#include "sdkconfig.h"

static const char *TAG = "ws2812b-controller";
#define LED_RES_HZ (10 * 1000 * 1000) // 10 MHz

static led_strip_handle_t led_strip = NULL;
static SemaphoreHandle_t led_mutex = NULL;
static uint16_t led_count = 0;

bool ws2812b_init(uint8_t gpio_num, uint16_t leds_count) {
    // Проверяем, не инициализирована ли уже лента
    if (led_strip != NULL) {
        ESP_LOGW(TAG, "WS2812B strip already initialized");
        return true; // Считаем это успешной инициализацией
    }

    // Включаем GPIO pin 4
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << 4),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(CONFIG_WS2812B_ENABLE_GPIO, 1);
    ESP_LOGI(TAG, "Enabled GPIO pin WS2812B_ENABLE_GPIO");

    // Создаем мьютекс для безопасного доступа к ленте из разных задач
    if (led_mutex == NULL) {
        led_mutex = xSemaphoreCreateMutex();
        if (led_mutex == NULL) {
            ESP_LOGE(TAG, "Failed to create led mutex");
            return false;
        }
    }

    led_strip_config_t strip_config = {
        .strip_gpio_num = gpio_num,
        .max_leds = leds_count,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB,
        .led_model = LED_MODEL_WS2812,
        .flags.invert_out = false,
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = LED_RES_HZ,
        .mem_block_symbols = 0,
    };

    ESP_LOGI(TAG, "Initializing WS2812B strip on GPIO %d with %d LEDs", gpio_num, leds_count);
    esp_err_t ret = led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create WS2812B strip (err: %d)", ret);
        return false;
    }

    led_count = leds_count;
    
    // Инициализируем все светодиоды в выключенное состояние
    ws2812b_clear_all();
    
    return true;
}

bool ws2812b_set_pixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b) {
    if (led_strip == NULL) {
        ESP_LOGE(TAG, "WS2812B strip not initialized");
        return false;
    }

    if (index >= led_count) {
        ESP_LOGW(TAG, "Invalid LED index: %d (max: %d)", index, led_count - 1);
        return false;
    }

    bool result = false;
    if (xSemaphoreTake(led_mutex, portMAX_DELAY) == pdTRUE) {
        esp_err_t ret = led_strip_set_pixel(led_strip, index, r, g, b);
        result = (ret == ESP_OK);
        xSemaphoreGive(led_mutex);
    }

    return result;
}

bool ws2812b_set_all_pixels(uint8_t r, uint8_t g, uint8_t b) {
    if (led_strip == NULL) {
        ESP_LOGE(TAG, "WS2812B strip not initialized");
        return false;
    }

    bool result = true;
    if (xSemaphoreTake(led_mutex, portMAX_DELAY) == pdTRUE) {
        for (uint16_t i = 0; i < led_count; i++) {
            esp_err_t ret = led_strip_set_pixel(led_strip, i, r, g, b);
            if (ret != ESP_OK) {
                result = false;
                ESP_LOGW(TAG, "Failed to set pixel %d (err: %d)", i, ret);
            }
        }
        xSemaphoreGive(led_mutex);
    } else {
        result = false;
    }

    return result;
}

bool ws2812b_clear_all(void) {
    return ws2812b_set_all_pixels(0, 0, 0) && ws2812b_refresh();
}

bool ws2812b_refresh(void) {
    if (led_strip == NULL) {
        ESP_LOGE(TAG, "WS2812B strip not initialized");
        return false;
    }

    bool result = false;
    if (xSemaphoreTake(led_mutex, portMAX_DELAY) == pdTRUE) {
        esp_err_t ret = led_strip_refresh(led_strip);
        result = (ret == ESP_OK);
        xSemaphoreGive(led_mutex);
    }

    return result;
}

bool ws2812b_deinit(void) {
    if (led_strip == NULL) {
        ESP_LOGW(TAG, "WS2812B strip not initialized, skipping deinit");
        return true;
    }

    // Выключаем все светодиоды перед деинициализацией
    ws2812b_clear_all();

    bool result = false;
    if (xSemaphoreTake(led_mutex, portMAX_DELAY) == pdTRUE) {
        esp_err_t ret = led_strip_del(led_strip);
        result = (ret == ESP_OK);
        led_strip = NULL;
        led_count = 0;
        xSemaphoreGive(led_mutex);
    }

    // Удаляем мьютекс
    if (led_mutex != NULL) {
        vSemaphoreDelete(led_mutex);
        led_mutex = NULL;
    }

    // Выключаем GPIO pin 4
    gpio_set_level(CONFIG_WS2812B_ENABLE_GPIO, 0);
    ESP_LOGI(TAG, "Disabled GPIO pin WS2812B_ENABLE_GPIO");

    ESP_LOGI(TAG, "WS2812B deinit %s", result ? "successful" : "failed");
    return result;
}

bool ws2812b_is_initialized(void) {
    return (led_strip != NULL);
}

uint16_t ws2812b_get_led_count(void) {
    return led_count;
}

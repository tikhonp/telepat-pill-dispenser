#include "esp_err.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "iot_button.h"
#include "medsenger_http_requests.h"
#include <stdint.h>
#include <sys/time.h>

static const char *TAG = "BUTTON";

#define BUTTON_PIN 6

static void _submit(void *params) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    ESP_ERROR_CHECK(mhr_submit_succes_cell((uint32_t)tv.tv_sec, 1));
    vTaskDelete(NULL);
}

static void button_single_click_cb(void *arg, void *usr_data) {
    ESP_LOGI(TAG, "BUTTON_SINGLE_CLICK");
    xTaskCreate(&_submit, "submit-pills-task", 8192, NULL, 1, NULL);
}

void button_init() {
    // create gpio button
    button_config_t gpio_btn_cfg = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = CONFIG_BUTTON_LONG_PRESS_TIME_MS,
        .short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME_MS,
        .gpio_button_config =
            {
                .gpio_num = BUTTON_PIN,
                .active_level = 0,
            },
    };
    button_handle_t gpio_btn = iot_button_create(&gpio_btn_cfg);
    if (NULL == gpio_btn) {
        ESP_LOGE(TAG, "Button create failed");
    }

    iot_button_register_cb(gpio_btn, BUTTON_SINGLE_CLICK,
                           button_single_click_cb, NULL);
}

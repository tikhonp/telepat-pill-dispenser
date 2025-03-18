#include "esp_log.h"
#include "iot_button.h"
#include "submit-request.h"
#include <stdint.h>
#include <sys/time.h>

static const char *TAG = "BUTTON";

#define BUTTON_PIN 6

static void button_single_click_cb(void *arg, void *usr_data) {
    ESP_LOGI(TAG, "BUTTON_SINGLE_CLICK");

    struct timeval tv;
    gettimeofday(&tv, NULL);

    run_submit_pills_task((uint32_t)tv.tv_sec, 2);
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

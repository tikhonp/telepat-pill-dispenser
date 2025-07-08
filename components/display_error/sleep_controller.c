#include "sleep_controller.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "hal/gpio_types.h"
#include "led_controller.h"
#include "medsenger_refresh_rate.h"
#include "schedule_data.h"
#include "sdkconfig.h"
#include <stdint.h>
#include <sys/param.h>
#include <sys/time.h>

#define RESET_BUTTON_MASK (1ULL << CONFIG_RESET_BUTTON_PIN)

void de_sleep(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    // find nearest cell
    uint32_t nearest_cell_time = 0;

    for (int i = 0; i < CONFIG_SD_CELLS_COUNT; ++i) {
        sd_cell_schedule_t cell = sd_get_schedule_by_cell_indx(i);
        if (cell.start_timestamp > tv.tv_sec &&
            (cell.start_timestamp < nearest_cell_time ||
             nearest_cell_time == 0))
            nearest_cell_time = cell.start_timestamp;
    }

    uint32_t medsenger_refresh_rate;
    esp_err_t err = m_get_medsenger_refresh_rate_sec(&medsenger_refresh_rate);
    if (err != ESP_OK) {
        ESP_LOGE("SLEEP", "Failed to get medsenger refresh rate: %s",
                 esp_err_to_name(err));
        medsenger_refresh_rate =
            CONFIG_CDC_DEFAULT_REFRESH_RATE_SEC; // default value
    }

    de_stop_blinking();

    uint64_t wakeup_time_sec;
    if (nearest_cell_time != 0)
        wakeup_time_sec =
            MIN(medsenger_refresh_rate, nearest_cell_time - tv.tv_sec);
    else
        wakeup_time_sec = medsenger_refresh_rate;

    gpio_config_t wakeup_button_conf = {
        .pin_bit_mask = RESET_BUTTON_MASK,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&wakeup_button_conf);

    ESP_LOGI("SLEEP", "Enabling timer wakeup, %" PRIu64 "s\n", wakeup_time_sec);
    ESP_ERROR_CHECK(
        esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000 * 1000));
    ESP_ERROR_CHECK(esp_deep_sleep_enable_gpio_wakeup(RESET_BUTTON_MASK, 0));
    esp_deep_sleep_start();
}

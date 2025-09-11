#include "battery_controller.h"
#include "button_controller.h"
#include "buzzer.h"
#include "cell_activity_watchdog.h"
#include "cell_led_controller.h"
#include "cleaner.h"
#include "connect.h"
#include "display_error.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: export
#include "freertos/task.h"
#include "init_global_manager.h"
#include "led_controller.h"
#include "medsenger_http_requests.h"
#include "medsenger_synced.h"
#include "nvs_flash.h"
#include "schedule_data.h"
#include "sdkconfig.h"
#include "send_event_data.h"
#include "sleep_controller.h"
#include <stddef.h>
#include <sys/time.h>

#define TAG "telepat-pill-dispenser"

/* Variable holding number of times ESP32 restarted since first boot.
 * It is placed into RTC memory using RTC_DATA_ATTR and
 * maintains its value when ESP32 wakes from deep sleep.
 */
RTC_DATA_ATTR static int boot_count = 0;

#ifndef CONFIG_FLASH_DEFAULT_WIFI_CREDS

#define EVENTS_COUNT_BUFFER 64

static void send_saved_on_flash_events() {
    esp_err_t err;
    se_send_event_t events[EVENTS_COUNT_BUFFER],
        new_events[EVENTS_COUNT_BUFFER];
    size_t elements_count, new_elements_count = 0;

    err = se_get_fsent_events(events, EVENTS_COUNT_BUFFER, &elements_count);
    ESP_ERROR_CHECK(err);

    ESP_LOGI(TAG, "Send save on flash elements: %zu", elements_count);

    if (elements_count == 0)
        return;

    for (int i = 0; i < elements_count; ++i) {
        err = mhr_submit_succes_cell(events[i].timestamp, events[i].cell_indx);
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "Failed to send saved data to Medsenger");
            new_events[new_elements_count++] = events[i];
        }
    }

    err = se_save_fsent_events(new_events, new_elements_count);
    ESP_ERROR_CHECK(err);
}

static void main_flow(void) {
    ESP_LOGI(TAG, "Starting pill-dispenser...");

    // TODO: remove led deinit
    cdc_deinit_led_signals();
    cdc_init_led_signals();

    battery_monitor_init();
    ESP_LOGI(TAG, "Battery voltage: %d mV\n", battery_monitor_read_voltage());

    // Initialize NVS, network and freertos
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    bool debug_boot = false;

    if (cause == ESP_SLEEP_WAKEUP_GPIO) {
        ESP_LOGI(TAG, "Woke up by EXT1 (reset button).");

        debug_boot = true;

        gpio_config_t io_conf = {.pin_bit_mask = 1ULL
                                                 << CONFIG_RESET_BUTTON_PIN,
                                 .mode = GPIO_MODE_INPUT,
                                 .pull_up_en = GPIO_PULLUP_ENABLE,
                                 .pull_down_en = GPIO_PULLDOWN_DISABLE,
                                 .intr_type = GPIO_INTR_DISABLE};
        gpio_config(&io_conf);

        if (gpio_get_level(CONFIG_RESET_BUTTON_PIN) == 0) {
            ESP_LOGI(TAG, "Button pressed, waiting %d ms to confirm hold...",
                     CONFIG_RESET_HOLD_TIME_MS);
            de_start_blinking(DE_STAY_HOLDING);

            int elapsed = 0;
            while (gpio_get_level(CONFIG_RESET_BUTTON_PIN) == 0 &&
                   elapsed < CONFIG_RESET_HOLD_TIME_MS) {
                vTaskDelay(pdMS_TO_TICKS(10));
                elapsed += 10;
            }

            de_stop_blinking();

            if (elapsed >= CONFIG_RESET_HOLD_TIME_MS) {
                de_start_blinking(DE_RED);
                vTaskDelay(pdMS_TO_TICKS(
                    1000)); // Give some time for the button press to be registered
                ESP_LOGI(TAG, "Button held for %d ms. Resetting NVS.",
                         CONFIG_RESET_HOLD_TIME_MS);
                nvs_clean_all();
            } else {
                ESP_LOGI(TAG, "Button was released before timeout.");
                de_start_blinking(DE_GREEN); // green
            }
        } else {
            ESP_LOGI(TAG, "Button not pressed.");
        }
    }
    if (debug_boot) {
        ESP_LOGI(TAG, "Debug boot enabled");
        de_start_blinking(DE_GREEN);
        vTaskDelay(pdMS_TO_TICKS(
            1000)); // Give some time for the button press to be registered
        de_stop_blinking();
    }
    ESP_LOGI(TAG, "wakeup cause: %d", cause);

    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    schedule_data_init();
    global_manager_init();
    buzzer_init();
    button_controller_init();
    display_error_init();

    ESP_LOGI(TAG, "Connecting to Wi-Fi...");
    if (wm_connect() != ESP_OK) {
        if (debug_boot) {
            ESP_LOGE(TAG, "Failed to connect to wi-fi");
            de_start_blinking(DE_RED);
            vTaskDelay(pdMS_TO_TICKS(2000));
            ESP_LOGI(TAG, "Call stop blinking");
            de_stop_blinking();
            ESP_LOGI(TAG, "Continuing in debug mode");
        }
        gm_set_medsenger_synced(false);
        ESP_LOGE(TAG, "Failed to connect to wi-fi");
        err = sd_load_schedule_from_flash();
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "Failed to load schedule from flash");
            display_error(DE_STAY_HOLDING);
        }
    } else {
        if (debug_boot) {
            de_start_blinking(DE_WIFI_CONNECTED);
            vTaskDelay(pdMS_TO_TICKS(2000));
            de_stop_blinking();
        }
        if (mhr_fetch_schedule(&sd_save_schedule) != ESP_OK) {
            gm_set_medsenger_synced(false);
            ESP_LOGE(TAG, "Failed to fetch medsenger schedule");
            de_start_blinking(DE_SYNC_FAILED);
            err = sd_load_schedule_from_flash();
            if (err != ESP_OK) {
                ESP_LOGI(TAG, "Failed to load schedule from flash");
                display_error(DE_STAY_HOLDING);
            }
        }
    }
    if (gm_get_medsenger_synced())
        send_saved_on_flash_events();
    sd_print_schedule();

    vTaskDelay(pdMS_TO_TICKS(2000));

    err = cdc_monitor();
    if (err != ESP_OK)
        while (1)
            vTaskDelay(pdMS_TO_TICKS(1000));
    else
        ESP_LOGI(TAG, "PREPARE SLEEP");

    ESP_ERROR_CHECK(wm_disconnect());
    cdc_deinit_led_signals();
    de_sleep();
}
#endif

#ifdef CONFIG_FLASH_DEFAULT_WIFI_CREDS
static void flash_default_wifi_credentials() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    wifi_creds_t creds = {
        .ssid = CONFIG_DEFAULT_WIFI_SSID,
        .psk = CONFIG_DEFAULT_WIFI_PSK,
    };
    ESP_LOGI(TAG, "Flashing default Wi-Fi credentials: ssid='%s', psk='%s'",
             creds.ssid, creds.psk);
    ESP_ERROR_CHECK(gm_set_wifi_creds(&creds));
    ESP_LOGI(TAG, "Default Wi-Fi credentials flashed");
}
#endif

void app_main(void) {
    ++boot_count;
    ESP_LOGI(TAG, "Boot count: %d", boot_count);
#ifdef CONFIG_FLASH_DEFAULT_WIFI_CREDS
    flash_default_wifi_credentials();
#else
    main_flow();
#endif
}

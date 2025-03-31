#include "button_controller.h"
#include "buzzer.h"
#include "cell_activity_watchdog.h"
#include "cell_led_controller.h"
#include "connect.h"
#include "display_error.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: export
#include "init_global_manager.h"
#include "medsenger_http_requests.h"
#include "medsenger_synced.h"
#include "nvs_flash.h"
#include "schedule_data.h"
#include "send_event_data.h"
#include <stddef.h>
#include <stdint.h>
#include <sys/time.h>

#define TAG "telepat-pill-dispenser"

/* Variable holding number of times ESP32 restarted since first boot.
 * It is placed into RTC memory using RTC_DATA_ATTR and
 * maintains its value when ESP32 wakes from deep sleep.
 */
RTC_DATA_ATTR static int boot_count = 0;

#define EVENTS_COUNT_BUFFER 64

static void send_saved_on_flash_events() {
    esp_err_t err;
    se_send_event_t events[EVENTS_COUNT_BUFFER],
        new_events[EVENTS_COUNT_BUFFER];
    size_t elements_count, new_elements_count = 0;

    err = se_get_fsent_events(events, EVENTS_COUNT_BUFFER, &elements_count);
    ESP_ERROR_CHECK(err);

    ESP_LOGI(TAG, "Send save on flash elements: %d", elements_count);

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

    // Initialize NVS, network and freertos
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    sd_init();
    gm_init();
    b_init();
    bc_init();
    de_init();
    cdc_init_led_signals();

    if (wm_connect() != ESP_OK) {
        gm_set_medsenger_synced(false);
        ESP_LOGE(TAG, "Failed to connect to wi-fi");
        err = sd_load_schedule_from_flash();
        if (err != ESP_OK) {
            de_display_error(FLASH_LOAD_FAILED);
        }
    } else if (mhr_fetch_schedule(&sd_save_schedule) != ESP_OK) {
        gm_set_medsenger_synced(false);
        ESP_LOGE(TAG, "Failed to fetch medsenger schedule");
        err = sd_load_schedule_from_flash();
        if (err != ESP_OK) {
            de_display_error(FLASH_LOAD_FAILED);
        }
    }
    if (gm_get_medsenger_synced())
        send_saved_on_flash_events();
    sd_print_schedule();

    vTaskDelay(2000 / portTICK_PERIOD_MS);

    err = cdc_monitor();
    if (err != ESP_OK)
        while (1)
            vTaskDelay(1000 / portTICK_PERIOD_MS);
    else
        ESP_LOGI(TAG, "PREPARE SLEEP");
}

void app_main(void) {
    ++boot_count;
    ESP_LOGI(TAG, "Boot count: %d", boot_count);

    main_flow();
}

#include "display_error.h"
#include "button_controller.h"
#include "buzzer.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/idf_additions.h"
#include "led_controller.h"
#include "stop_all_tasks.h"

const static char *TAG = "display-error-queue";

static QueueHandle_t de_display_error_tasks;
static const uint8_t de_queue_len = 1;

static void de_run_display_error(void *params) {
    de_error_code_t error;
    while (1) {
        if (xQueueReceive(de_display_error_tasks, (void *)&error,
                          portMAX_DELAY) == pdTRUE) {
            break;
        }
    }

    ESP_LOGE(TAG, "Fired error display for error code: %d", error);

    gm_fire_stop_all_tasks();

    ESP_LOGI(TAG, "Starting display error for %d", error);
    de_start_blinking(error);

    b_play_notification(FATAL_ERROR);

    // Add a short delay so error indication is visible before waiting for button
    vTaskDelay(pdMS_TO_TICKS(1500)); // 1.5 seconds

    ESP_LOGI(TAG, "Waiting for button press to reset");
    bc_wait_for_single_press();
    ESP_LOGI(TAG, "Button pressed, stopping blinking");

    ESP_LOGI(TAG, "Restarting device after error indication");
    esp_restart();

    vTaskDelete(NULL);
}

void display_error(de_error_code_t error) {
    ESP_LOGE(TAG,
             "Display error triggered"); // Clear log when error is requested
    xQueueGenericSend(de_display_error_tasks, (void *)&error, 1,
                      queueSEND_TO_BACK);
}

void display_error_init(void) {
    de_display_error_tasks = xQueueGenericCreate(
        de_queue_len, sizeof(de_error_code_t), queueQUEUE_TYPE_SET);
    if (de_display_error_tasks == NULL) {
        ESP_LOGE(TAG, "Play tasks queue creation failed");
        abort();
    }
    xTaskCreate(&de_run_display_error, "display-error-task", 4096, NULL, 1,
                NULL);
}

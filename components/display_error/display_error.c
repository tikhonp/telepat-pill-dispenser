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
    de_fatal_error_t error;
    while (1) {
        if (xQueueReceive(de_display_error_tasks, (void *)&error,
                          portMAX_DELAY) == pdTRUE) {
            break;
        }
    }

    ESP_LOGE(TAG, "Fired");

    gm_fire_stop_all_tasks();

    de_start_blinking();

    b_play_notification(FATAL_ERROR);

    bc_wait_for_single_press();

    esp_restart();

    vTaskDelete(NULL);
}

void de_display_error(de_fatal_error_t error) {
    xQueueGenericSend(de_display_error_tasks, (void *)&error, 1,
                      queueSEND_TO_BACK);
}

void de_init(void) {
    de_display_error_tasks = xQueueGenericCreate(
        de_queue_len, sizeof(de_fatal_error_t), queueQUEUE_TYPE_SET);
    if (de_display_error_tasks == NULL) {
        ESP_LOGE(TAG, "Play tasks queue creation failed");
        abort();
    }
    xTaskCreate(&de_run_display_error, "display-error-task", 4096, NULL, 1,
                NULL);
}

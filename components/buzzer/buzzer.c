#include "buzzer.h"
#include "buzzer_player_private.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "portmacro.h"
#include <stdlib.h>

const static char *TAG = "BUZZER-QUEUE";

static QueueHandle_t b_buzzer_play_tasks;
static const uint8_t b_buzzer_queue_len = 5;

static void run_play_tasks_listener(void *pvParameters) {
    enum b_notification_t notification;
    while (1) {
        if (xQueueReceive(b_buzzer_play_tasks, (void *)&notification,
                          portMAX_DELAY) == pdTRUE) {
            b_play_notification_task(notification);
        }
    }
    vTaskDelete(NULL);
}

void b_init(void) {
    b_buzzer_play_tasks = xQueueGenericCreate(b_buzzer_queue_len, sizeof(int),
                                              queueQUEUE_TYPE_SET);
    if (b_buzzer_play_tasks == NULL) {
        ESP_LOGE(TAG, "Play tasks queue creation failed");
        abort();
    }

    b_buzzer_channel_init();

    xTaskCreate(&run_play_tasks_listener, "play-tasks-listener", 4096, NULL, 1,
                NULL);
}

esp_err_t b_play_notification(enum b_notification_t notification) {
    if (xQueueGenericSend(b_buzzer_play_tasks, (void *)&notification,
                          portMAX_DELAY, queueSEND_TO_BACK) != pdTRUE) {
        ESP_LOGE(TAG, "Play tasks queue full");
        return ESP_FAIL;
    }
    return ESP_OK;
}

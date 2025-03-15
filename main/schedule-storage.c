#include "schedule-storage.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: export
#include "freertos/semphr.h"
#include "sdkconfig.h"
#include <stdint.h>

static SemaphoreHandle_t mu;
uint32_t timestamps[CONFIG_CELLS_COUNT] = {0};

void init_schedule() {
    mu = xSemaphoreCreateMutex();
    if (mu == NULL) {
        ESP_LOGE("SEMFR", "Failed to create mutex");
        abort();
    }
}

void update_schedule(uint32_t *t) {
    for (int i = 0; i < CONFIG_CELLS_COUNT; i++) {
        if (xSemaphoreTake(mu, pdMS_TO_TICKS(100)) != pdTRUE) {
            ESP_LOGE("SCHDU", "Timeout on semaphore take");
        } else {
            timestamps[i] = t[i];
            xSemaphoreGive(mu);
        }
    }
}

uint32_t schedule_get(int i) {
    uint32_t t = 0;
    if (i >= CONFIG_CELLS_COUNT) {
        ESP_LOGE("SCHEDULE", "Requested ivalid schedule index: %d", i);
        return t;
    }
    if (xSemaphoreTake(mu, portMAX_DELAY)) {
        t = timestamps[i];
        xSemaphoreGive(mu);
    }
    return t;
}

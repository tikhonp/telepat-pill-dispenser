#include "cells.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: export
#include "freertos/task.h"
#include "schedule_data.h"
#include <stdint.h>
#include <sys/time.h>
#include <time.h>

static const char *TAG = "SCHEDULER";
#define LED_PIN 2

void retrieve_time() {

    struct timeval tv;
    gettimeofday(&tv, NULL);

#if !CONFIG_IDF_TARGET_LINUX
    for (int i = 0; i < 4; i++) {
        sd_cell_schedule_t cell = sd_get_schedule_by_cell_indx(i);
        if ((cell.start_timestamp <= (uint32_t)tv.tv_sec) &&
            cell.end_timestamp >= (uint32_t)tv.tv_sec) {
            enable_cell(i);
        } else {
            disable_cell(i);
        }
    }
#endif

    /*time_t now;*/
    /*char strftime_buf[64];*/
    /*struct tm timeinfo;*/
    /**/
    /*time(&now);*/
    /*// Europe/Moscow*/
    /*setenv("TZ", "CST-3", 1);*/
    /*tzset();*/
    /**/
    /*localtime_r(&now, &timeinfo);*/
    /*strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);*/
    /*ESP_LOGI(TAG, "The current date/time in ??? is: %s", strftime_buf);*/
}

void scheduler_task(void *pvParameters) {
    while (1) {
        retrieve_time();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    ESP_LOGI(TAG, "Finished");
#if !CONFIG_IDF_TARGET_LINUX
    vTaskDelete(NULL);
#endif
}

void run_scheduler_task() {
    xTaskCreate(&scheduler_task, "http_test_task", 8192, NULL, 1, NULL);
}

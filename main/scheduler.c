#include "cells.h"
#include "esp_log.h"
#include "esp_netif_sntp.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: export
#include "freertos/task.h"
#include "schedule-storage.h"
#include "sdkconfig.h"
#include <stdint.h>
#include <sys/time.h>
#include <time.h>

static const char *TAG = "SCHEDULER";
#define LED_PIN 2

void retrieve_time() {

    struct timeval tv;
    gettimeofday(&tv, NULL);

#if !CONFIG_IDF_TARGET_LINUX
    for (int i = 0; i < CONFIG_CELLS_COUNT; i++)
        if ((schedule_get(i) - (uint32_t)tv.tv_sec) < 15) {
            enable_cell(i);
        } else {
            disable_cell(i);
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
    xTaskCreate(&scheduler_task, "http_test_task", 8192, NULL, 5, NULL);
}

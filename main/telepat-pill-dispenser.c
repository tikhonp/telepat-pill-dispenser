#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: export
#include "nvs_flash.h"
#include "wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "http-request.h"

#define TAG "telepat-pill-dispenser"

#define LED_PIN 2

void app_main(void) {
    ESP_LOGI(TAG, "Hello world!");

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();

    xTaskCreate(&http_test_task, "http_test_task", 8192, NULL, 5, NULL);

    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    while (1) {
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

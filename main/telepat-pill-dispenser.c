#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: export

#include "secrets.h"
#include "wifi_connection.h"

#include "lwip/sockets.h"
#include <sys/types.h>

#define TAG "telepat-pill-dispenser"

#define LED_PIN 2
#define TCP_SUCCESS 1 << 0
#define TCP_FAILURE 1 << 1

esp_err_t connect_tcp_server(void) {
    struct sockaddr_in serverInfo = {0};
    char readBuffer[1024] = {0};

    in_addr_t addr;
    inet_pton(AF_INET, "192.168.2.246", &addr);

    serverInfo.sin_family = AF_INET;
    serverInfo.sin_addr.s_addr = addr;
    serverInfo.sin_port = htons(12345);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        ESP_LOGE(TAG, "Failed to create a socket..?");
        return TCP_FAILURE;
    }

    if (connect(sock, (struct sockaddr *)&serverInfo, sizeof(serverInfo)) !=
        0) {
        ESP_LOGE(TAG, "Failed to connect to %s!",
                 inet_ntoa(serverInfo.sin_addr.s_addr));
        close(sock);
        return TCP_FAILURE;
    }

    ESP_LOGI(TAG, "Connected to TCP server.");

    while (1) {
        bzero(readBuffer, sizeof(readBuffer));
        int r = read(sock, readBuffer, sizeof(readBuffer) - 1);
        for (int i = 0; i < r; i++) {
            putchar(readBuffer[i]);
        }
    }

    return TCP_SUCCESS;
}

void app_main(void) {
    esp_err_t status = connect_to_appppp();

    status = connect_tcp_server();
    if (TCP_SUCCESS != status) {
        ESP_LOGI(TAG, "Failed to connect to remote server, dying...");
        return;
    }

    ESP_LOGI(TAG, "Hello world!");

    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    while (1) {
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}


#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: export

#include "lwip/sockets.h"
#include <sys/types.h>

#define TAG "tcp_socket"

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


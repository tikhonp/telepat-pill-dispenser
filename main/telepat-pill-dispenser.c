#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: export
/*#include "esp_system.h"*/
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "secrets.h"

/*#include "lwip/err.h"*/
#include "lwip/sockets.h"
#include <sys/types.h>
/*#include "lwip/sys.h"*/
/*#include "lwip/netdb.h"*/
/*#include "lwip/dns.h"*/

#define TAG "telepat-pill-dispenser"

#define LED_PIN 2
#define WIFI_SUCCESS 1 << 0
#define WIFI_FAILURE 1 << 1
#define TCP_SUCCESS 1 << 0
#define TCP_FAILURE 1 << 1
#define MAX_FAILURES 10

// event group to contain status information
static EventGroupHandle_t wifi_event_group;

// retry tracker
static int s_retry_num = 0;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Connecting to AP...");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT &&
               event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAX_FAILURES) {
            ESP_LOGI(TAG, "Reconnecting to AP...");
            esp_wifi_connect();
            s_retry_num++;
        } else {
            xEventGroupSetBits(wifi_event_group, WIFI_FAILURE);
        }
    }
}

static void ip_event_handler(void *arg, esp_event_base_t event_base,
                             int32_t event_id, void *event_data) {
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "STA IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(wifi_event_group, WIFI_SUCCESS);
    }
}

esp_err_t connect_wifi() {

    int status = WIFI_FAILURE;

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /** EVENT LOOP CRAZINESS **/
    wifi_event_group = xEventGroupCreate();

    esp_event_handler_instance_t wifi_handler_event_instance;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL,
        &wifi_handler_event_instance));

    esp_event_handler_instance_t got_ip_event_instance;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL,
        &got_ip_event_instance));

    /** START THE WIFI DRIVER **/
    wifi_config_t wifi_config = {
        .sta =
            {
                .ssid = WIFI_SSID,
                .password = WIFI_PASSWORD,
                .threshold.authmode = WIFI_AUTH_WPA2_PSK,
                .pmf_cfg = {.capable = true, .required = false},
            },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "STA initialization complete");

    /** NOW WE WAIT **/
    EventBits_t bits =
        xEventGroupWaitBits(wifi_event_group, WIFI_SUCCESS | WIFI_FAILURE,
                            pdFALSE, pdFALSE, portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we
     * can test which event actually happened. */
    if (bits & WIFI_SUCCESS) {
        ESP_LOGI(TAG, "Connected to ap");
        status = WIFI_SUCCESS;
    } else if (bits & WIFI_FAILURE) {
        ESP_LOGI(TAG, "Failed to connect to ap");
        status = WIFI_FAILURE;
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
        status = WIFI_FAILURE;
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(
        IP_EVENT, IP_EVENT_STA_GOT_IP, got_ip_event_instance));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(
        WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_handler_event_instance));
    vEventGroupDelete(wifi_event_group);

    return status;
}

esp_err_t connect_tcp_server(void) {
    struct sockaddr_in serverInfo = {0};
    char readBuffer[1024] = {0};

    in_addr_t addr;
    inet_pton(AF_INET, "192.168.2.24", &addr);

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

    esp_err_t status = WIFI_FAILURE;

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    status = connect_wifi();
    if (WIFI_SUCCESS != status) {
        ESP_LOGI(TAG, "Failed to associate to AP, dying...");
        return;
    }

    wifi_ap_record_t wifidata;
    if (esp_wifi_sta_get_ap_info(&wifidata) == 0) {
        printf("rssi:%d\r\n", wifidata.rssi);
    }

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


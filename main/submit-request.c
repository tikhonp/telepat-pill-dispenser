#include "esp_err.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "medsenger_http_requests_private.h"
#include "sdkconfig.h"
#include <stdint.h>

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048
#define BITMASK(start, length) ((uint32_t)0xffffffff >> (32 - length)) << start
#define REQUEST_BODY_BUFFER_SIZE 50

static const char *TAG = "SUBMIT_PILLS_REQUEST";
static const char *ser_number = CONFIG_SERIAL_NUMBER;

void create_body(uint32_t timestamp, uint8_t cell_indx, char *buffer,
                 int buffer_length) {
    uint32_t mask = BITMASK(0, 8);
    buffer[0] = (uint8_t)(((timestamp) >> (3 * 8)) & mask);
    buffer[1] = (uint8_t)(((timestamp) >> (2 * 8)) & mask);
    buffer[2] = (uint8_t)(((timestamp) >> (1 * 8)) & mask);
    buffer[3] = (uint8_t)(timestamp & mask);
    buffer[4] = cell_indx;
    strncpy(buffer + 5, ser_number, buffer_length - 5);
}

static void submit_pills(const char *post_data) {
    esp_http_client_config_t config = {
        .host = CONFIG_HTTP_ENDPOINT,
        .path = "/submit",
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .method = HTTP_METHOD_POST,
        .cert_pem = howsmyssl_com_root_cert_pem_start,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_http_client_set_header(client, "Content-Type",
                               "application/octet-stream");
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTPS Status = %d, content_length = %" PRId64,
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
    free((void *)post_data);
}

static uint32_t req_timestamp;
static uint8_t req_cell_indx;

void submit_pills_task(void *pvParameters) {
    char *post_buffer = malloc(sizeof(char) * (REQUEST_BODY_BUFFER_SIZE + 1));
    create_body(req_timestamp, req_cell_indx, post_buffer,
                REQUEST_BODY_BUFFER_SIZE);
    submit_pills(post_buffer);
    ESP_LOGI(TAG, "Finished");
#if !CONFIG_IDF_TARGET_LINUX
    vTaskDelete(NULL);
#endif
}

void run_submit_pills_task(uint32_t timestamp, uint8_t cell_indx) {
    req_timestamp = timestamp;
    req_cell_indx = cell_indx;
    xTaskCreate(&submit_pills_task, "submit-pills-task", 8192, NULL, 5, NULL);
}

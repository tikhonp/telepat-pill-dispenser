#include "esp_http_client.h"
#include "esp_log.h"
#include "medsenger_http_requests.h"
#include "medsenger_http_requests_private.h"
#include "pilld_common.h"
#include <string.h>

static const char *TAG = "SUBMIT_PILLS_REQUEST";

static const char *mhr_ser_number = CONFIG_SERIAL_NUMBER;

static void mhr_construct_submit_success_body(uint32_t timestamp,
                                              uint8_t cell_indx, char *buffer,
                                              int buffer_length) {
    uint32_t mask = BITMASK(0, 8);
    buffer[0] = (uint8_t)(((timestamp) >> (3 * 8)) & mask);
    buffer[1] = (uint8_t)(((timestamp) >> (2 * 8)) & mask);
    buffer[2] = (uint8_t)(((timestamp) >> (1 * 8)) & mask);
    buffer[3] = (uint8_t)(timestamp & mask);
    buffer[4] = cell_indx;
    strncpy(buffer + 5, mhr_ser_number, buffer_length - 5);
}

static esp_err_t mhr_submit_succes_cell_do_request(uint32_t timestamp,
                                                   uint8_t cell_indx) {

    char *post_data = malloc(sizeof(char) * (REQUEST_BODY_BUFFER_SIZE + 1));

    mhr_construct_submit_success_body(timestamp, cell_indx, post_data,
                                      REQUEST_BODY_BUFFER_SIZE);

    esp_http_client_config_t config = {
        .host = CONFIG_HTTP_ENDPOINT,
        .path = CONFIG_SUBMIT_SUCCESS_QUERY_PATH,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .method = HTTP_METHOD_POST,
        .cert_pem = howsmyssl_com_root_cert_pem_start,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_http_client_set_header(client, "Content-Type",
                               "application/octet-stream");

    esp_err_t err = esp_http_client_perform(client);

    esp_http_client_cleanup(client);
    free(post_data);
    return err;
}

esp_err_t mhr_submit_succes_cell(uint32_t timestamp, uint8_t cell_indx) {
    esp_err_t err;
    int mhr_retries = CONFIG_REQUEST_RETRY_COUNT + 1;
    for (; mhr_retries > 0; --mhr_retries) {
        err = mhr_submit_succes_cell_do_request(timestamp, cell_indx);
        if (err == ESP_OK)
            return ESP_OK;
        ESP_LOGE(TAG, "Error perform http request %s. Retrying %d / %d",
                 esp_err_to_name(err),
                 CONFIG_REQUEST_RETRY_COUNT + 1 - mhr_retries,
                 CONFIG_REQUEST_RETRY_COUNT);
    }
    return err;
}

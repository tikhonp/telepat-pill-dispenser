#include "battery_controller.h"
#include "esp_err.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_tls.h"
#include "medsenger_http_requests.h"
#include "medsenger_http_requests_private.h"
#include "medsenger_refresh_rate.h"
#include "pilld_common.h"
#include <stdint.h>
#include <stdlib.h>

static mhr_schedule_handler mhr_handler;
static esp_err_t mhr_handler_errored;

static const char *TAG = "GET_SCHEDULE_HTTP_REQUEST";

static esp_err_t _mhr_http_event_handler(esp_http_client_event_t *evt) {
    static char *output_buffer; // Buffer to store response of http request from
                                // event handler
    static int output_len;      // Stores number of bytes read
    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key,
                 evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        // Clean the buffer in case of a new request
        if (output_len == 0 && evt->user_data) {
            // we are just starting to copy the output data into the use
            memset(evt->user_data, 0, MAX_HTTP_OUTPUT_BUFFER);
        }
        /*
         *  Check for chunked encoding is added as the URL for chunked encoding
         * used in this example returns binary data. However, event handler can
         * also be used in case chunked encoding is used.
         */
        if (!esp_http_client_is_chunked_response(evt->client)) {
            // If user_data buffer is configured, copy the response into the
            // buffer
            int copy_len = 0;
            if (evt->user_data) {
                // The last byte in evt->user_data is kept for the NULL
                // character in case of out-of-bound access.
                copy_len =
                    MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
                if (copy_len) {
                    memcpy(evt->user_data + output_len, evt->data, copy_len);
                }
            } else {
                int content_len =
                    esp_http_client_get_content_length(evt->client);
                if (output_buffer == NULL) {
                    // We initialize output_buffer with 0 because it is used by
                    // strlen() and similar functions therefore should be null
                    // terminated.
                    output_buffer =
                        (char *)calloc(content_len + 1, sizeof(char));
                    output_len = 0;
                    if (output_buffer == NULL) {
                        ESP_LOGE(TAG,
                                 "Failed to allocate memory for output buffer");
                        return ESP_FAIL;
                    }
                }
                copy_len = MIN(evt->data_len, (content_len - output_len));
                if (copy_len) {
                    memcpy(output_buffer + output_len, evt->data, copy_len);
                }
            }
            output_len += copy_len;
        }

        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        if (output_buffer != NULL) {
            esp_err_t err =
                mhr_handler(output_buffer, output_len - sizeof(uint32_t));
            ESP_ERROR_CHECK(
                m_save_medsenger_refresh_rate_sec(decode_fixed32_little_end(
                    output_buffer + output_len - sizeof(uint32_t))));
            free(output_buffer);
            output_buffer = NULL;
            mhr_handler_errored = err;
            return err;
        }
        output_len = 0;
        return ESP_OK;
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        int mbedtls_err = 0;
        esp_err_t err = esp_tls_get_and_clear_last_error(
            (esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
        if (err != 0) {
            ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
            ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
        }
        if (output_buffer != NULL) {
            free(output_buffer);
            output_buffer = NULL;
        }
        output_len = 0;
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
        esp_http_client_set_redirection(evt->client);
        break;
    }
    return ESP_OK;
}

static esp_err_t mhr_fetch_schedule_do_request(mhr_schedule_handler h,
                                               int battery_voltage,
                                               const char *serial_nu) {
    mhr_handler = h;
    char query_buffer[128];
    int needed = snprintf(query_buffer, sizeof(query_buffer),
                          "serial_number=%s&battery_voltage=%d", serial_nu,
                          battery_voltage);
    if (needed < 0 || needed >= sizeof(query_buffer)) {
        ESP_LOGE(TAG, "Failed to create query string for HTTP request");
        return ESP_ERR_INVALID_ARG;
    }
    esp_http_client_config_t config = {
        .host = CONFIG_HTTP_ENDPOINT,
        .path = CONFIG_FETCH_SCHEDULE_QUERY_PATH,
        .query = query_buffer,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .event_handler = _mhr_http_event_handler,
        .cert_pem = howsmyssl_com_root_cert_pem_start,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);
    esp_http_client_cleanup(client);
    return err;
}

esp_err_t mhr_fetch_schedule(mhr_schedule_handler h, const char *serial_nu) {
    int voltage = battery_monitor_read_voltage_mv();
    esp_err_t err;
    int mhr_retries = CONFIG_REQUEST_RETRY_COUNT + 1;
    for (; mhr_retries > 0; --mhr_retries) {
        mhr_handler_errored = ESP_OK;
        err = mhr_fetch_schedule_do_request(h, voltage, serial_nu);
        if (err == ESP_OK) {
            if (mhr_handler_errored == ESP_OK)
                return ESP_OK;
            else {
                err = mhr_handler_errored;
            }
        }
        ESP_LOGE(TAG, "Error perform http request %s. Retrying %d / %d",
                 esp_err_to_name(err),
                 CONFIG_REQUEST_RETRY_COUNT + 1 - mhr_retries,
                 CONFIG_REQUEST_RETRY_COUNT);
    }
    return err;
}

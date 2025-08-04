#include "esp_err.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/idf_additions.h"
#include "wifi_manager_private.h"
#include <string.h>
#include "led_controller.h"

static const char *TAG = "wifi-manager";
static esp_netif_t *wm_s_sta_netif = NULL;
static SemaphoreHandle_t wm_s_semph_get_ip_addrs = NULL;

static int wm_s_retry_num = 0;

/**
 * @brief Checks the netif description if it contains specified prefix.
 * All netifs created within common connect component are prefixed with the
 * module TAG, so it returns true if the specified netif is owned by this module
 */
bool wm_is_our_netif(const char *prefix, esp_netif_t *netif) {
    return strncmp(prefix, esp_netif_get_desc(netif), strlen(prefix) - 1) == 0;
}

static void wm_handler_on_wifi_disconnect(void *arg,
                                          esp_event_base_t event_base,
                                          int32_t event_id, void *event_data) {
    wm_s_retry_num++;
    if (wm_s_retry_num > CONFIG_WM_WIFI_CONN_MAX_RETRY) {
        ESP_LOGI(TAG, "WiFi Connect failed %d times, stop reconnect.", wm_s_retry_num);
        // stop LED blinking on failure
        de_stop_blinking();
        /* let wifi_sta_do_connect() return */
        if (wm_s_semph_get_ip_addrs) {
            xSemaphoreGive(wm_s_semph_get_ip_addrs);
        }
        wm_wifi_sta_do_disconnect();
        return;
    }
    wifi_event_sta_disconnected_t *disconn = event_data;
    if (disconn->reason == WIFI_REASON_ROAMING) {
        ESP_LOGD(TAG, "station roaming, do nothing");
        return;
    }
    ESP_LOGI(TAG, "Wi-Fi disconnected %d, trying to reconnect...",
             disconn->reason);
    esp_err_t err = esp_wifi_connect();
    if (err == ESP_ERR_WIFI_NOT_STARTED) {
        return;
    }
    ESP_ERROR_CHECK(err);
}

static void wm_handler_on_wifi_connect(void *esp_netif,
                                       esp_event_base_t event_base,
                                       int32_t event_id, void *event_data) {
    // nothing here yet
}

static void wm_handler_on_sta_got_ip(void *arg, esp_event_base_t event_base,
                                     int32_t event_id, void *event_data) {
    wm_s_retry_num = 0;
    // stop LED blinking on successful connection
    de_stop_blinking();
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    if (!wm_is_our_netif(WM_NETIF_DESC_STA, event->esp_netif)) {
        return;
    }
    ESP_LOGI(TAG, "Got IPv4 event: Interface \"%s\" address: " IPSTR,
             esp_netif_get_desc(event->esp_netif), IP2STR(&event->ip_info.ip));
    if (wm_s_semph_get_ip_addrs) {
        xSemaphoreGive(wm_s_semph_get_ip_addrs);
    } else {
        ESP_LOGI(TAG, "- IPv4 address: " IPSTR ",", IP2STR(&event->ip_info.ip));
    }
}

void wm_wifi_start(void) {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_netif_inherent_config_t esp_netif_config =
        ESP_NETIF_INHERENT_DEFAULT_WIFI_STA();
    esp_netif_config.if_desc = WM_NETIF_DESC_STA;
    esp_netif_config.route_prio = 128;
    wm_s_sta_netif = esp_netif_create_wifi(WIFI_IF_STA, &esp_netif_config);
    esp_wifi_set_default_wifi_sta_handlers();

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void wm_wifi_stop(void) {
    esp_err_t err = esp_wifi_stop();
    if (err == ESP_ERR_WIFI_NOT_INIT) {
        return;
    }
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(esp_wifi_deinit());
    ESP_ERROR_CHECK(
        esp_wifi_clear_default_wifi_driver_and_handlers(wm_s_sta_netif));
    esp_netif_destroy(wm_s_sta_netif);
    wm_s_sta_netif = NULL;
}

esp_err_t wm_wifi_sta_do_connect(wifi_config_t wifi_config, bool wait) {
    if (wait) {
        wm_s_semph_get_ip_addrs = xSemaphoreCreateBinary();
        if (wm_s_semph_get_ip_addrs == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }
    wm_s_retry_num = 0;
    ESP_ERROR_CHECK(
        esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED,
                                   &wm_handler_on_wifi_disconnect, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wm_handler_on_sta_got_ip, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(
        WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &wm_handler_on_wifi_connect,
        wm_s_sta_netif));

    ESP_LOGI(TAG, "Connecting to %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    esp_err_t ret = esp_wifi_connect();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi connect failed! Error: %s", esp_err_to_name(ret));
        return ret;
    }
    if (wait) {
        ESP_LOGI(TAG, "Waiting for IP(s)");
        xSemaphoreTake(wm_s_semph_get_ip_addrs, portMAX_DELAY);
        vSemaphoreDelete(wm_s_semph_get_ip_addrs);
        wm_s_semph_get_ip_addrs = NULL;
        if (wm_s_retry_num > CONFIG_WM_WIFI_CONN_MAX_RETRY) {
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}

esp_err_t wm_wifi_sta_do_disconnect(void) {
    ESP_ERROR_CHECK(
        esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED,
                                     &wm_handler_on_wifi_disconnect));
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                 &wm_handler_on_sta_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_unregister(
        WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &wm_handler_on_wifi_connect));
    return esp_wifi_disconnect();
}

void wm_wifi_shutdown(void) {
    wm_wifi_sta_do_disconnect();
    wm_wifi_stop();
}

esp_err_t wm_wifi_connect(wifi_creds_t creds) {
    ESP_LOGI(TAG, "Start wifi_connect.");
    wm_wifi_start();
    wifi_config_t wifi_config = {
        .sta =
            {
                .scan_method = WM_WIFI_SCAN_METHOD,
                .sort_method = WM_WIFI_CONNECT_AP_SORT_METHOD,
                .threshold.rssi = CONFIG_WM_WIFI_SCAN_RSSI_THRESHOLD,
                .threshold.authmode = WM_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            },
    };
    strncpy((char *)wifi_config.sta.ssid, creds.ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, creds.psk,
            sizeof(wifi_config.sta.password));
    return wm_wifi_sta_do_connect(wifi_config, true);
}

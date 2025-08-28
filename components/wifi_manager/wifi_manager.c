#include "esp_err.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/idf_additions.h"
#include "wifi_manager_private.h"
#include <string.h>

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

const char *wifi_reason_to_name(wifi_err_reason_t reason) {
    switch (reason) {
    case WIFI_REASON_UNSPECIFIED:
        return "1, Unspecified reason";
    case WIFI_REASON_AUTH_EXPIRE:
        return "2, Authentication expired";
    case WIFI_REASON_AUTH_LEAVE:
        return "3, Deauthentication due to leaving";
    case WIFI_REASON_DISASSOC_DUE_TO_INACTIVITY:
        return "4, Disassociated due to inactivity";
    case WIFI_REASON_ASSOC_TOOMANY:
        return "5, Too many associated stations";
    case WIFI_REASON_CLASS2_FRAME_FROM_NONAUTH_STA:
        return "6, Class 2 frame received from nonauthenticated STA";
    case WIFI_REASON_CLASS3_FRAME_FROM_NONASSOC_STA:
        return "7, Class 3 frame received from nonassociated STA";
    case WIFI_REASON_ASSOC_LEAVE:
        return "8, Deassociated due to leaving";
    case WIFI_REASON_ASSOC_NOT_AUTHED:
        return "9, Association but not authenticated";
    case WIFI_REASON_DISASSOC_PWRCAP_BAD:
        return "10, Disassociated due to poor power capability";
    case WIFI_REASON_DISASSOC_SUPCHAN_BAD:
        return "11, Disassociated due to unsupported channel";
    case WIFI_REASON_BSS_TRANSITION_DISASSOC:
        return "12, Disassociated due to BSS transition";
    case WIFI_REASON_IE_INVALID:
        return "13, Invalid Information Element (IE";
    case WIFI_REASON_MIC_FAILURE:
        return "14, MIC failure";
    case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
        return "15, 4-way handshake timeout";
    case WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT:
        return "16, Group key update timeout";
    case WIFI_REASON_IE_IN_4WAY_DIFFERS:
        return "17, IE differs in 4-way handshake";
    case WIFI_REASON_GROUP_CIPHER_INVALID:
        return "18, Invalid group cipher";
    case WIFI_REASON_PAIRWISE_CIPHER_INVALID:
        return "19, Invalid pairwise cipher";
    case WIFI_REASON_AKMP_INVALID:
        return "20, Invalid AKMP";
    case WIFI_REASON_UNSUPP_RSN_IE_VERSION:
        return "21, Unsupported RSN IE version";
    case WIFI_REASON_INVALID_RSN_IE_CAP:
        return "22, Invalid RSN IE capabilities";
    case WIFI_REASON_802_1X_AUTH_FAILED:
        return "23, 802.1X authentication failed";
    case WIFI_REASON_CIPHER_SUITE_REJECTED:
        return "24, Cipher suite rejected";
    case WIFI_REASON_TDLS_PEER_UNREACHABLE:
        return "25, TDLS peer unreachable";
    case WIFI_REASON_TDLS_UNSPECIFIED:
        return "26, TDLS unspecified";
    case WIFI_REASON_SSP_REQUESTED_DISASSOC:
        return "27, SSP requested disassociation";
    case WIFI_REASON_NO_SSP_ROAMING_AGREEMENT:
        return "28, No SSP roaming agreement";
    case WIFI_REASON_BAD_CIPHER_OR_AKM:
        return "29, Bad cipher or AKM";
    case WIFI_REASON_NOT_AUTHORIZED_THIS_LOCATION:
        return "30, Not authorized in this location";
    case WIFI_REASON_SERVICE_CHANGE_PERCLUDES_TS:
        return "31, Service change precludes TS";
    case WIFI_REASON_UNSPECIFIED_QOS:
        return "32, Unspecified QoS reason";
    case WIFI_REASON_NOT_ENOUGH_BANDWIDTH:
        return "33, Not enough bandwidth";
    case WIFI_REASON_MISSING_ACKS:
        return "34, Missing ACKs";
    case WIFI_REASON_EXCEEDED_TXOP:
        return "35, Exceeded TXOP";
    case WIFI_REASON_STA_LEAVING:
        return "36, Station leaving";
    case WIFI_REASON_END_BA:
        return "37, End of Block Ack (BA";
    case WIFI_REASON_UNKNOWN_BA:
        return "38, Unknown Block Ack (BA";
    case WIFI_REASON_TIMEOUT:
        return "39, Timeout";
    case WIFI_REASON_PEER_INITIATED:
        return "46, Peer initiated disassociation";
    case WIFI_REASON_AP_INITIATED:
        return "47, AP initiated disassociation";
    case WIFI_REASON_INVALID_FT_ACTION_FRAME_COUNT:
        return "48, Invalid FT action frame count";
    case WIFI_REASON_INVALID_PMKID:
        return "49, Invalid PMKID";
    case WIFI_REASON_INVALID_MDE:
        return "50, Invalid MDE";
    case WIFI_REASON_INVALID_FTE:
        return "51, Invalid FTE";
    case WIFI_REASON_TRANSMISSION_LINK_ESTABLISH_FAILED:
        return "67, Transmission link establishment failed";
    case WIFI_REASON_ALTERATIVE_CHANNEL_OCCUPIED:
        return "68, Alternative channel occupied";
    case WIFI_REASON_BEACON_TIMEOUT:
        return "200, Beacon timeout";
    case WIFI_REASON_NO_AP_FOUND:
        return "201, No AP found";
    case WIFI_REASON_AUTH_FAIL:
        return "202, Authentication failed";
    case WIFI_REASON_ASSOC_FAIL:
        return "203, Association failed";
    case WIFI_REASON_HANDSHAKE_TIMEOUT:
        return "204, Handshake timeout";
    case WIFI_REASON_CONNECTION_FAIL:
        return "205, Connection failed";
    case WIFI_REASON_AP_TSF_RESET:
        return "206, AP TSF reset";
    case WIFI_REASON_ROAMING:
        return "207, Roaming";
    case WIFI_REASON_ASSOC_COMEBACK_TIME_TOO_LONG:
        return "208, Association comeback time too long";
    case WIFI_REASON_SA_QUERY_TIMEOUT:
        return "209, SA query timeout";
    case WIFI_REASON_NO_AP_FOUND_W_COMPATIBLE_SECURITY:
        return "210, No AP found with compatible security";
    case WIFI_REASON_NO_AP_FOUND_IN_AUTHMODE_THRESHOLD:
        return "211, No AP found in auth mode threshold";
    case WIFI_REASON_NO_AP_FOUND_IN_RSSI_THRESHOLD:
        return "212, No AP found in RSSI threshold";
    default:
        return "Unknown reason";
    }
}

static void wm_handler_on_wifi_disconnect(void *arg,
                                          esp_event_base_t event_base,
                                          int32_t event_id, void *event_data) {
    wm_s_retry_num++;
    if (wm_s_retry_num > CONFIG_WM_WIFI_CONN_MAX_RETRY) {
        ESP_LOGI(TAG, "WiFi Connect failed %d times, stop reconnect.",
                 wm_s_retry_num);
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
    ESP_LOGI(TAG, "Wi-Fi disconnected %s, trying to reconnect...",
             wifi_reason_to_name(disconn->reason));
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
    esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED,
                                 &wm_handler_on_wifi_disconnect);
    esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                 &wm_handler_on_sta_got_ip);
    esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED,
                                 &wm_handler_on_wifi_connect);
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
    strncpy((char *)wifi_config.sta.ssid, creds.ssid,
            sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, creds.psk,
            sizeof(wifi_config.sta.password));
    return wm_wifi_sta_do_connect(wifi_config, true);
}

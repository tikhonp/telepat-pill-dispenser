#include "captive_portal.h"
#include "button_controller.h"
#include "cleaner.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: export
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "lwip/ip4_addr.h"
#include "lwip/sockets.h"
#include "sdkconfig.h"
#include <string.h>

static const char *TAG = "WIFI-CONFIGURATOR";

static EventGroupHandle_t s_event_group;
static wifi_credentials_t s_user_data = {0};
#define WIFI_PORTAL_EVENT_SUCCESS BIT0

#define CAPTIVE_URL "http://192.168.0.1/"

// --- DNS сервер (всегда отвечает 192.168.0.1) ---
static void dns_server_task(void *pvParameters) {
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create DNS socket");
        vTaskDelete(NULL);
        return;
    }
    struct sockaddr_in source_addr, dest_addr;
    socklen_t socklen = sizeof(source_addr);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(53);
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        ESP_LOGE(TAG, "DNS socket bind failed");
        close(sock);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "Captive DNS server started");

    uint8_t buf[512];
    while (1) {
        int len = recvfrom(sock, buf, sizeof(buf), 0,
                           (struct sockaddr *)&source_addr, &socklen);
        if (len > 0) {
            ESP_LOGI(TAG, "DNS request received, length=%d", len);
            uint8_t response[512];
            memcpy(response, buf, len);
            response[2] |= 0x80; // QR = response
            response[3] |= 0x80; // RA = 1
            response[7] = 1;     // ANCOUNT = 1
            int i = 12;
            while (i < len && buf[i])
                i += buf[i] + 1;
            i += 5;
            memcpy(response + i,
                   "\xc0\x0c\x00\x01\x00\x01\x00\x00\x00\x3c\x00\x04", 12);
            response[i + 12] = 192;
            response[i + 13] = 168;
            response[i + 14] = 0;
            response[i + 15] = 1;
            sendto(sock, response, i + 16, 0, (struct sockaddr *)&source_addr,
                   socklen);
        }
    }
    close(sock);
    vTaskDelete(NULL);
}

// --- Captive portal HTML ---
static const char *captive_html =
    "<!DOCTYPE html><html lang='ru'><head><meta charset='utf-8'/>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>WiFi Portal</title>"
    "<style>"
    "body{background:#181c20;color:#fff;font-family:'Segoe "
    "UI',Arial,sans-serif;margin:0;display:flex;justify-content:center;align-"
    "items:center;height:100vh;}"
    ".card{background:#222c36;padding:2rem "
    "2.5rem;border-radius:16px;box-shadow:0 2px 12px "
    "#0008;width:100%;max-width:360px;text-align:center;}"
    ".card "
    "h1{font-size:1.5rem;margin-bottom:1.5rem;font-weight:600;letter-spacing:0."
    "05em;}"
    ".input-group{margin-bottom:1.2rem;text-align:left;}"
    ".input-group label{display:block;margin-bottom:0.35em;font-size:1em;}"
    ".input-group input{width:100%;padding:0.65em "
    "0.9em;border-radius:8px;border:1px solid "
    "#3b4452;font-size:1em;background:#181c20;color:#fff;outline:none;}"
    ".btn{background:#4fa3f7;color:#fff;border:none;padding:0.85em "
    "2em;font-size:1.08em;font-weight:500;border-radius:8px;cursor:pointer;"
    "transition:.2s;}"
    ".btn:disabled{opacity:0.6;cursor:not-allowed;}"
    ".btn:hover:not(:disabled){background:#2176c7;}"
    ".msg{margin-top:1.2rem;font-size:1em;min-height:1.2em;}"
    "</style>"
    "<script>"
    "function onSubmitForm(e){"
    " var btn=document.getElementById('submitBtn');"
    " btn.disabled=true;"
    " document.getElementById('msg').textContent='Отправка данных...';"
    "}"
    "window.onload=function(){"
    " var form=document.getElementById('wifiform');"
    " if(form) form.onsubmit=onSubmitForm;"
    "};"
    "</script>"
    "</head>"
    "<body><form id='wifiform' class='card' method='POST' action='/'>"
    "<h1>Подключить устройство MedsengerTM к WiFi</h1>"
    "<div class='input-group'><label for='ssid'>Имя сети (SSID)</label>"
    "<input id='ssid' name='ssid' maxlength='32' required "
    "autocomplete='off'></div>"
    "<div class='input-group'><label for='password'>Пароль</label>"
    "<input id='password' name='password' maxlength='64' type='password' "
    "required></div>"
    "<button class='btn' id='submitBtn' type='submit'>Подключиться</button>"
    "<div class='msg' id='msg'>Введите данные вашей Wi-Fi сети</div>"
    "</form></body></html>";

// --- Обработчик GET /
static esp_err_t portal_root_get_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "GET request for '/' - returning captive page.");
    esp_err_t err = httpd_resp_set_type(req, "text/html");
    if (err != ESP_OK)
        return err;
    return httpd_resp_send(req, captive_html, HTTPD_RESP_USE_STRLEN);
}

// --- Обработчик системных запросов для проверки живости сети ---
static esp_err_t portal_redirect_handler(httpd_req_t *req) {
    esp_err_t err;
    err = httpd_resp_set_status(req, "302 Found");
    if (err != ESP_OK)
        return err;
    err = httpd_resp_set_hdr(req, "Location", CAPTIVE_URL);
    if (err != ESP_OK)
        return err;
    err = httpd_resp_send(req, NULL, 0);
    if (err != ESP_OK)
        return err;
    return ESP_OK;
}

// --- AP Event Handler ---
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event =
            (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac),
                 event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event =
            (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d, reason=%d",
                 MAC2STR(event->mac), event->aid, event->reason);
    }
}

#if CONFIG_WM_WIFI_AUTH_OPEN
#define WM_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_WM_WIFI_AUTH_WEP
#define WM_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define WM_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_WM_WIFI_AUTH_WPA2_PSK
#define WM_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_WM_WIFI_AUTH_WPA_WPA2_PSK
#define WM_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_WM_WIFI_AUTH_WPA2_ENTERPRISE
#define WM_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_ENTERPRISE
#elif CONFIG_WM_WIFI_AUTH_WPA3_PSK
#define WM_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_WM_WIFI_AUTH_WPA2_WPA3_PSK
#define WM_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_WM_WIFI_AUTH_WAPI_PSK
#define WM_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

// --- Проверка подключения STA ---
static bool check_sta_connect(const char *ssid, const char *password) {
    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, password,
            sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WM_WIFI_SCAN_AUTH_MODE_THRESHOLD,
    wifi_config.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    // Обязательно делаем disconnect перед connect (иначе будет ESP_ERR_WIFI_CONN)
    ESP_ERROR_CHECK(esp_wifi_disconnect());
    vTaskDelay(pdMS_TO_TICKS(100));
    esp_err_t err = esp_wifi_connect();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_connect failed: %s", esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(TAG, "Try connect STA to SSID: %s and PSK: '%s'", ssid, password);

    // Ждем до 10 секунд на подключение
    for (int i = 0; i < 20; ++i) {
        wifi_ap_record_t info;
        if (esp_wifi_sta_get_ap_info(&info) == ESP_OK) {
            ESP_LOGI(TAG, "STA connected to %s!", ssid);
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    ESP_LOGW(TAG, "STA connect timeout or failed");
    return false;
}

/* in-place URL-decode: converts %XX and +.
   Safe to run on a buffer that contains the urlencoded value.
*/
static void url_decode_inplace(char *s) {
    char *src = s;
    char *dst = s;

    while (*src) {
        if (*src == '%') {
            /* expect two hex digits after % */
            if (isxdigit((unsigned char)src[1]) &&
                isxdigit((unsigned char)src[2])) {
                char hex[3] = {src[1], src[2], 0};
                *dst++ = (char)strtol(hex, NULL, 16);
                src += 3;
            } else {
                /* malformed % sequence -> copy literally */
                *dst++ = *src++;
            }
        } else if (*src == '+') {
            /* plus becomes space in application/x-www-form-urlencoded */
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

/* --- POST-запрос: только парсинг и сигнал ---
   (изменения: url_decode_inplace вызов, безопасное копирование в s_user_data) */
static esp_err_t portal_post_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "POST request - URI: %s", req->uri);

    esp_err_t err;
    char buf[128] = {0};
    int total_len = req->content_len;
    int received = 0, ret;
    while (received < total_len && received < (int)sizeof(buf) - 1) {
        ret = httpd_req_recv(req, buf + received, total_len - received);
        if (ret <= 0)
            break;
        received += ret;
    }
    buf[received] = 0;

    /* Парсим форму (application/x-www-form-urlencoded) */
    char ssid[33] = {0}, password[65] = {0};
    err = httpd_query_key_value(buf, "ssid", ssid, sizeof(ssid));
    if (err != ESP_OK)
        return err;
    err = httpd_query_key_value(buf, "password", password, sizeof(password));
    if (err != ESP_OK)
        return err;

    /* Декодируем percent-encoding */
    url_decode_inplace(ssid);
    url_decode_inplace(password);

    /* Безопасное копирование в пользовательскую структуру */
    snprintf(s_user_data.ssid, sizeof(s_user_data.ssid), "%s", ssid);
    snprintf(s_user_data.psk, sizeof(s_user_data.psk), "%s", password);

    ESP_LOGI(TAG, "User requested WiFi SSID: %s", s_user_data.ssid);
    /* НЕ логируйте пароль в продакшене. Только для отладки: */
    ESP_LOGD(TAG, "User requested WiFi Password (decoded): %s",
             s_user_data.psk);

    /* Сигнал основной задаче на попытку подключения */
    xEventGroupSetBits(s_event_group, WIFI_PORTAL_EVENT_SUCCESS);

    /* Отправляем страницу с JS-индикатором */
    err = httpd_resp_set_type(req, "text/html");
    if (err != ESP_OK)
        return err;
    return httpd_resp_sendstr(
        req,
        "<html><head><meta charset='UTF-8'></head><body "
        "style='background:#181c20;color:#fff;font-family:sans-serif;"
        "display:flex;height:100vh;align-items:center;justify-content:center;'>"
        "<div "
        "style='background:#222c36;padding:2em;border-radius:12px;font-size:1."
        "1em;'>"
        "Пробуем подключиться к сети...<br>Ожидайте ответа"
        "<script>setTimeout(function(){"
        "document.body.innerHTML='<div "
        "style=\\'color:#ffb0b0;font-size:1.2em;background:#222c36;border-"
        "radius:12px;padding:2em;text-align:center;\\'>"
        "Точка доступа не доступна! Проверьте правильность данных или "
        "попробуйте позже.<br><a "
        "style=\\'color:#4fa3f7;text-decoration:underline;margin-top:1em;"
        "display:inline-block;\\' href=\\'\\' "
        "onclick=\\'window.location.href=\"/\";return false;\\'>Попробовать "
        "ещё раз</a></div>';"
        "},11000);</script>"
        "</div></body></html>");
}

// --- 404 обработчик ---
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t httperr) {
    esp_err_t err;
    err = httpd_resp_set_status(req, "302 Temporary Redirect");
    if (err != ESP_OK)
        return err;
    err = httpd_resp_set_hdr(req, "Location", "/");
    if (err != ESP_OK)
        return err;
    err = httpd_resp_send(req, "Redirect to the captive portal",
                          HTTPD_RESP_USE_STRLEN);
    if (err != ESP_OK)
        return err;
    ESP_LOGI(TAG, "Redirecting to root");
    return ESP_OK;
}

void register_redirect_uris(httpd_handle_t server) {
    const char *paths[] = {
        "/generate_204", // Android
        "/gen_204",
        "/ncsi.txt",                  // Windows
        "/library/test/success.html", // Apple
        "/hotspot-detect.html",       // iOS/macOS
        "/success.txt"                // Some Huawei devices
    };

    for (int i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i) {
        httpd_uri_t uri = {.uri = paths[i],
                           .method = HTTP_GET,
                           .handler = portal_redirect_handler,
                           .user_ctx = NULL};
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri));
    }
}

static void start_captive_portal_httpd(httpd_handle_t *server) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;

    if (httpd_start(server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return;
    }

    httpd_uri_t root_uri = {.uri = "/",
                            .method = HTTP_GET,
                            .handler = portal_root_get_handler,
                            .user_ctx = NULL};
    ESP_ERROR_CHECK(httpd_register_uri_handler(*server, &root_uri));

    httpd_uri_t post_handler = {.uri = "/", // POST только на корень
                                .method = HTTP_POST,
                                .handler = portal_post_handler,
                                .user_ctx = NULL};
    ESP_ERROR_CHECK(httpd_register_uri_handler(*server, &post_handler));

    ESP_ERROR_CHECK(httpd_register_err_handler(*server, HTTPD_404_NOT_FOUND,
                                               http_404_error_handler));

    register_redirect_uris(*server);
}

static void wait_for_button_to_set_default_creads(void *pvParameters) {
    bc_wait_for_single_press();
    flash_default_wifi_credentials();
}

// --- Главная функция captive-портала: блокирует до успеха или рестарта ---
wifi_credentials_t start_wifi_captive_portal(const char *ap_ssid,
                                             const char *ap_password) {
    memset(&s_user_data, 0, sizeof(s_user_data));
    s_event_group = xEventGroupCreate();

    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();

    // Статический IP для AP
    esp_netif_ip_info_t ip_info;
    ip4_addr_t ip;
    ip4addr_aton(CAPTIVE_PORTAL_IP, &ip);
    ip_info.ip.addr = ip.addr;
    ip4addr_aton(CAPTIVE_PORTAL_GW, &ip);
    ip_info.gw.addr = ip.addr;
    ip4addr_aton(CAPTIVE_PORTAL_NETMASK, &ip);
    ip_info.netmask.addr = ip.addr;
    ESP_ERROR_CHECK(esp_netif_dhcps_stop(ap_netif));
    ESP_ERROR_CHECK(esp_netif_set_ip_info(ap_netif, &ip_info));
    ESP_ERROR_CHECK(esp_netif_dhcps_start(ap_netif));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.ap.ssid, ap_ssid, sizeof(wifi_config.ap.ssid));
    wifi_config.ap.ssid_len = strlen(ap_ssid);
    strncpy((char *)wifi_config.ap.password, ap_password,
            sizeof(wifi_config.ap.password));
    wifi_config.ap.channel = 1;
    wifi_config.ap.max_connection = 4;
    wifi_config.ap.authmode =
        (strlen(ap_password) == 0) ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;
    wifi_config.ap.pmf_cfg.required = false;
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // DNS сервер
    xTaskCreatePinnedToCore(dns_server_task, "dns_server", 4096, NULL, 5, NULL,
                            0);

    // HTTP сервер
    httpd_handle_t server = NULL;
    start_captive_portal_httpd(&server);

    TaskHandle_t button_task_h = NULL;
    xTaskCreate(&wait_for_button_to_set_default_creads, "button_task", 2048,
                NULL, 5, &button_task_h);

    // Цикл — пока не получим валидные данные и не подключимся к STA
    while (1) {
        xEventGroupWaitBits(s_event_group, WIFI_PORTAL_EVENT_SUCCESS, pdTRUE,
                            pdFALSE, portMAX_DELAY);
        if (check_sta_connect(s_user_data.ssid, s_user_data.psk)) {
            s_user_data.success = true;
            ESP_LOGI(TAG, "WiFi credentials are valid, stopping AP...");
            if (server)
                httpd_stop(server);
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
            // ESP_ERROR_CHECK(
            //     esp_wifi_set_config(WIFI_IF_AP, NULL)); // отключить AP
            ESP_ERROR_CHECK(esp_wifi_stop());
            break;
        } else {
            s_user_data.success = false;
            ESP_LOGW(TAG, "WiFi credentials invalid, still in AP mode.");
            // Всё остальное — на стороне браузера (через JS таймер — показывает ошибку)
        }
    }
    vTaskDelete(button_task_h);
    return s_user_data;
}

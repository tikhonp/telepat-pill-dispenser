#pragma once

#include <stdbool.h>

#define CAPTIVE_PORTAL_IP      "192.168.0.1"
#define CAPTIVE_PORTAL_GW      "192.168.0.1"
#define CAPTIVE_PORTAL_NETMASK "255.255.255.0"

typedef struct {
    char ssid[33];
    char psk[65];
    bool success;
} wifi_credentials_t;

/**
 * Запускает точку доступа ESP32, captive-портал и процесс авторизации пользователя.
 * @param ap_ssid SSID для точки доступа ESP32
 * @param ap_password Пароль для точки доступа ESP32 (может быть пустым)
 * @return Структура wifi_credentials_t: ssid, password, success-флаг.
 *         Если success=true, то ssid/password содержат выбранные данные для WiFi STA.
 */
wifi_credentials_t start_wifi_captive_portal(const char *ap_ssid, const char *ap_password);

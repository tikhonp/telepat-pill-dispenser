# WiFi configuration using captive portal

## Functions:
```c
start_wifi_captive_portal("<Configuration AP name>", "<Configuration AP password, nothing for no password>");
```

## Example:
```c
wifi_credentials_t creds = start_wifi_captive_portal("esp32test", "");
if (creds.success) {
    printf("WiFi creds: %s / %s\n", creds.ssid, creds.password);
} else {
    printf("Connection failed\n");
}
```

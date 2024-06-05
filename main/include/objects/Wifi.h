#ifndef WIFI_OBJ_H_
#define WIFI_OBJ_H_
#include "../main.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/err.h"
#include "lwip/ip4_addr.h"
#include "lwip/sys.h"
#include "nvs_flash.h"
#include "utils/debug.h"
#include "utils/scanner.h"
#include "esp_http_client.h"
#define WIFI_FINISHED           0b1000
#define WIFI_CONNECTED_BIT      0b0100
#define WIFI_WRONG_PASSWORD_BIT 0b0010
#define WIFI_ACTIVE_BIT         0b0001
#define WIFI_TIMEOUT            60000

#define TAG_NAME "Wifi Object"

#define EXAMPLE_ESP_WIFI_SSID CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_WIFI_CHANNEL CONFIG_ESP_WIFI_CHANNEL
#define EXAMPLE_MAX_STA_CONN CONFIG_ESP_MAX_STA_CONN

#ifndef ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD
#endif

#ifndef ESP_WIFI_SAE_MODE
#define ESP_WIFI_SAE_MODE
#endif

#ifndef CONFIG_ESP_MAXIMUM_RETRY
#define CONFIG_ESP_MAXIMUM_RETRY 7
#endif

#define EXAMPLE_ESP_MAXIMUM_RETRY CONFIG_ESP_MAXIMUM_RETRY

#if CONFIG_ESP_WPA3_SAE_PWE_HUNT_AND_PECK
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
#define EXAMPLE_H2E_IDENTIFIER ""
#elif CONFIG_ESP_WPA3_SAE_PWE_HASH_TO_ELEMENT
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HASH_TO_ELEMENT
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#elif CONFIG_ESP_WPA3_SAE_PWE_BOTH
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#endif
#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif


enum WifiState { CONNECTED = 1, CONNECTING = 0, DISCONNECTED = -1, NOT_INITIALIZED = -2 };


class Wifi {
    WifiState wifiState = NOT_INITIALIZED;
    esp_netif_t* sta_netif = NULL;
    esp_netif_t* ap_netif = NULL;
public:
    Wifi();
    virtual ~Wifi();
    WifiState connectToWifi(String& ssid, String& password);
    void mode(wifi_mode_t mode);

    WifiState getWifiState(WifiSettings* settings);
    esp_err_t getIPInfo(esp_netif_ip_info_t* ip_info);
    esp_err_t changeStationSettings(String& ssid, String& password);
    esp_err_t downloadFile(String url, String filename);
};
#endif  // WIFI_OBJ_H_
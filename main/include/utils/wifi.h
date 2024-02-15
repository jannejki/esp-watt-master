#if 0
#define WIFI_H_

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
#include "utils/file_serving_example_common.h"
#include "utils/scanner.h"

#define WIFI_CONNECTED_BIT      0b0100
#define WIFI_FINISHED           0b1000
#define WIFI_WRONG_PASSWORD_BIT 0b0010 

#define WIFI_TIMEOUT 60000

extern EventGroupHandle_t s_wifi_event_group;

enum WifiState { CONNECTED, DISCONNECTED, NOT_INITIALIZED };

#if 0
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data);

static void event_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data);
#endif
esp_err_t getIPInfo(esp_netif_ip_info_t* ip_info);
WifiState getWifiState(WifiSettings* settings);

esp_err_t initWifi();

EventBits_t connectToWifi(const String& ssid, const String& password);
#endif  // WIFI_H_
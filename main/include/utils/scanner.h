
#include <string.h>

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

#define DEFAULT_SCAN_LIST_SIZE 10
int wifi_scan(wifi_ap_record_t* found_ssids);
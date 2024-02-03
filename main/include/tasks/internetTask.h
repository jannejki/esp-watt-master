#ifndef INTERNET_TASK_H
#define INTERNET_TASK_H

#include <Arduino.h>
#include <objects/Wifi.h>

#include "../main.h"
#include "utils/scanner.h"
#include "utils/debug.h"

#include "../secrets.h"
#include "Preferences.h"
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
#include "utils/file_serving_example_common.h"

#define WIFI_SETTINGS_NAMESPACE "wifi"
#define WIFI_SETTINGS_SSID_KEY "ssid"
#define WIFI_SETTINGS_PASSWORD_KEY "password"

void internetTask(void* params);

#endif  // INTERNET_TASK_H
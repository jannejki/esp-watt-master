#ifndef INTERNET_TASK_H
#define INTERNET_TASK_H

#include <Arduino.h>

#include "../main.h"
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
#include "utils/debug.h"

#include "objects/Wifi.h"
#include "tasks/ledBlinkTask.h"
#include "utils/file_serving_example_common.h"
#include "utils/mqtt.h"
#include "driver/gpio.h"

#define WIFI_SETTINGS_NAMESPACE "wifi"
#define WIFI_SETTINGS_SSID_KEY "ssid"
#define WIFI_SETTINGS_PASSWORD_KEY "password"
#define ENABLE_SSL_CERT_DOWNLOADER CONFIG_ENABLE_SSL_CERT_DOWNLOADER
#define SSL_CERT_DOWNLOAD_URL CONFIG_SSL_CERT_DOWNLOAD_URL
#define TAG "internetTask"

void internetTask(void* params);

#endif  // INTERNET_TASK_H
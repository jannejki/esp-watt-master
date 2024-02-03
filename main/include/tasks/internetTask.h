#ifndef INTERNET_TASK_H
#define INTERNET_TASK_H

#include <Arduino.h>

#include <objects/Wifi.h>

#include "../main.h"
#include "../secrets.h"
#include "Preferences.h"

#define WIFI_SETTINGS_NAMESPACE "wifi"
#define WIFI_SETTINGS_SSID_KEY  "ssid"
#define WIFI_SETTINGS_PASSWORD_KEY "password"

void internetTask(void* params);

#endif  // INTERNET_TASK_H
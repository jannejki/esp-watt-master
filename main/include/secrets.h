/**
 * @file secrets.h
 * @brief This file contains the definitions of the secrets used in the project, like the WiFi SSID and password, the MQTT broker IP and port, and the device ID.
 * The secrets are defined in the src/secrets.cpp file.
*/

#ifndef SECRECTS_H
#define SECRECTS_H
#include "main.h"

extern String MQTT_BROKER;
extern String MQTT_PORT;
extern String DEVICE_ID;

extern String WIFI_SSID;
extern String WIFI_PASSWORD;
#endif  // SECRECTS_H
#pragma once
#ifndef MQTT_H
#define MQTT_H
#include "main.h"
#include "mqtt_client.h"
#include "objects/Wifi.h"


#define MQTT_BROKER_URL CONFIG_MQTT_BROKER_ADDRESS
#define MQTT_BROKER_PORT CONFIG_MQTT_BROKER_PORT
#define MQTT_PASSWORD CONFIG_MQTT_PASSWORD

#define MQTT_DISCONNECTED ( 1 << 0 )
#define MQTT_CONNECTED ( 1 << 1 )

bool mqtt_app_start();
#endif // MQTT_H
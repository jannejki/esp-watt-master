#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>

#define LED0 0
#define LED1 35

#define DEBUG_TASK (1 << 0)
#define INTERNET_TASK (1 << 4)

struct DebugMessage {
    char message[64];
    String sender = "unknown";
    TickType_t tick;
};

struct mqttMessage {
    String topic;
    String message;
};

enum relayState { on, off, noStateChange };

enum relayMode { automatic, manual, noModeChange };

struct RelaySettings {
    int relayNumber;
    enum relayState state;
    enum relayMode mode;
    double threshold;
};

struct WifiSettings {
    String ssid;
    String password;
    EventGroupHandle_t connectionFlag;
};

extern QueueHandle_t debugQueue;
extern QueueHandle_t mqttQueue;
extern QueueHandle_t relayQueue;
extern QueueHandle_t priceQueue;
extern QueueHandle_t wifiSettingsQueue;

extern EventGroupHandle_t taskInitializedGroup;
#endif  // MAIN_H
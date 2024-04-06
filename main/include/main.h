#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>

#define DEVICE_ID CONFIG_DEVICE_ID
#define MQTT_DEVICE_COMMAND_TOPIC CONFIG_MQTT_DEVICE_TOPIC_BASE "/" DEVICE_ID "/command"
#define MQTT_DEVICE_STATUS_TOPIC CONFIG_MQTT_DEVICE_TOPIC_BASE "/" DEVICE_ID "/status"

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
    char topic[64];
    char message[128];
};

struct ledBlinkTaskParams {
    uint8_t pin;
    int delay;
    bool* loopFinished;
};

enum relayState { on, off, noStateChange };

enum relayMode { automatic, manual, noModeChange };

struct RelaySettings {
    int relayNumber;
    enum relayState state;
    enum relayMode mode;
    double threshold;
};

/**
 * @brief struct for holding new ssid and password for wifi
 * @param ssid: new ssid where to connect
 * @param password: new password for the ssid
 * @param connectionFlag: event group for wifi connection. This will be used to signal the sender task that the wifi connection is finished. WIFI_FINSIHED, WIFI_CONNECTED_BIT, WIFI_WRONG_PASSWORD_BIT
*/
struct WifiSettings {
    String ssid;
    String password;
    EventGroupHandle_t connectionFlag; // WIFI_FINISHED, WIFI_CONNECTED_BIT, WIFI_WRONG_PASSWORD_BIT
};

extern QueueHandle_t debugQueue;
extern QueueHandle_t mqttReceiveQueue;
extern QueueHandle_t mqttTransmitQueue;
extern QueueHandle_t relayQueue;
extern QueueHandle_t priceQueue;
extern QueueHandle_t wifiSettingsQueue;

extern EventGroupHandle_t taskInitializedGroup;

extern EventGroupHandle_t mqttEventGroup;

#endif  // MAIN_H
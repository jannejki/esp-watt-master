#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include "esp_log.h"
#define DEVICE_ID CONFIG_DEVICE_ID
#define MQTT_DEVICE_COMMAND_TOPIC CONFIG_MQTT_DEVICE_TOPIC_BASE "/" DEVICE_ID "/command"
#define MQTT_DEVICE_STATUS_TOPIC CONFIG_MQTT_DEVICE_TOPIC_BASE "/" DEVICE_ID "/status"
#define MQTT_DEVICE_PING_TOPIC CONFIG_MQTT_DEVICE_TOPIC_BASE "/" DEVICE_ID "/ping"

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

enum relayState { on, off, noStateChange };

enum relayMode { automatic, manual, noModeChange };

struct RelaySettings {
    int relayNumber;
    enum relayState state;
    enum relayMode mode;
    double threshold;
    double price;
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


/**
 * @brief Enum for the display to know what part to update
*/
typedef enum {
    INTERNET_UPDATE = 0x01,
    RELAY_UPDATE = 0x03,
    MQTT_UPDATE = 0x05,
    ALL_UPDATES = 0x0F
} UpdateType;

typedef enum {
    STATION = 0x01,
    AP_MODE = 0x02
} InternetMode;

/**
 * @brief struct for holding the display message
 * @param updateType: type of the update. enum UpdateType
 * @param IPaddress: IP address of the device
 * @param connection: connection status
 * @param relays: Relay that is updated
*/
struct DisplayMessage {
    UpdateType updateType;
    InternetMode internetMode;
    char IPaddress[32];  // To hold the IP address as a string
    bool internetConnection;     // Connection status
    bool mqttConnection;
    RelaySettings relay;
};

extern QueueHandle_t debugQueue;
extern QueueHandle_t mqttReceiveQueue;
extern QueueHandle_t mqttTransmitQueue;
extern QueueHandle_t relayQueue;
extern QueueHandle_t priceQueue;
extern QueueHandle_t wifiSettingsQueue;
extern QueueHandle_t displayQueue;

extern EventGroupHandle_t taskInitializedGroup;
extern EventGroupHandle_t mqttEventGroup;

extern SemaphoreHandle_t sendRelayStatusSemaphore;

#endif  // MAIN_H
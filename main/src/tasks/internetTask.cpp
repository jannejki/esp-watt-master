#include "tasks/internetTask.h"
#define CONNECTION_ATTEMPT_TIMEOUT 2000

void debug(String message);
void setup_wifi();
void reconnect();
void callback(char* topic, byte* message, unsigned int length);

Preferences settings;
WiFiClient espClient;
PubSubClient client(espClient);

// SSID and password of Wifi connection:
const char* ssid = WIFI_SSID.c_str();
const char* password = WIFI_PASSWORD.c_str();

const char* mqtt_server = MQTT_BROKER.c_str();

void internetTask(void* params) {
    xEventGroupWaitBits(
        taskInitializedGroup, /* The event group being tested. */
        DEBUG_TASK,           /* The bits within the event group to wait for. */
        pdTRUE, /* DEBUG_TASK -bit should be cleared before returning. */
        pdTRUE, /* Don't wait for both bits, either bit will do. */
        portMAX_DELAY); /* Wait a maximum of 100ms for either bit to be set. */

    debug("internetTask started");

    setup_wifi();
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);

    while (1) {
        if (!client.connected()) {
            reconnect();
        }
        client.loop();
    }
}

void setup_wifi() {
    delay(10);
    // We start by connecting to a WiFi network
    String message = "Connecting to wifi";
    debug(message);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        debug(".");
    }

    debug("WiFi connected");
    debug("IP address: ");
    debug(WiFi.localIP().toString());
}

void callback(char* topic, byte* message, unsigned int length) {
    String debugString = "message length: " + String(length);
    debug(debugString);
    String messageTemp = "";

    for (int i = 0; i < length; i++) {
        messageTemp += (char)message[i];
    }

    mqttMessage msg;
    msg.message = messageTemp;
    msg.topic = topic;

    xQueueSend(mqttQueue, &msg, 0);

    debug(messageTemp);
}

void reconnect() {
    // Loop until we're reconnected or until timeout
    unsigned long startAttemptTime = millis();
    while (!client.connected() &&
           millis() - startAttemptTime < CONNECTION_ATTEMPT_TIMEOUT) {
        debug("Attempting MQTT connection...");
        // Attempt to connect
        if (client.connect(DEVICE_ID.c_str())) {
            debug("connected");
            // Subscribe
            client.subscribe("relay");
            client.subscribe("electric/price");
        } else {
            debug("failed, rc=");
            debug(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}
void debug(String message) {
    DebugMessage debug;
    debug.message = message;
    debug.tick = xTaskGetTickCount();
    debug.sender = "internetTask";

    xQueueSend(debugQueue, &debug, 0);
}

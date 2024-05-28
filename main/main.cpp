#include "main.h"

#include "esp_task_wdt.h"
#include "tasks/debugTask.h"
#include "tasks/internetTask.h"
#include "tasks/mqttTask.h"
#include "tasks/relayTask.h"

QueueHandle_t debugQueue;
QueueHandle_t mqttReceiveQueue;
QueueHandle_t mqttTransmitQueue;
QueueHandle_t relayQueue;
QueueHandle_t priceQueue;

/**
 * @brief Queue for sending new wifi settings to wifi task
 * Uses struct WifiSettings that contains new ssid, password and event group
 * for signaling the sender task that the wifi connection is finished
*/
QueueHandle_t wifiSettingsQueue;
EventBits_t uxBits;

EventGroupHandle_t taskInitializedGroup;
EventGroupHandle_t mqttEventGroup;

SemaphoreHandle_t sendRelayStatusSemaphore;

extern "C" void app_main() {
    initArduino();

    debugQueue = xQueueCreate(20, sizeof(DebugMessage));
    mqttReceiveQueue = xQueueCreate(10, sizeof(mqttMessage));
    mqttTransmitQueue = xQueueCreate(5, sizeof(mqttMessage));
    relayQueue = xQueueCreate(10, sizeof(RelaySettings));
    priceQueue = xQueueCreate(10, sizeof(double[6]));
    wifiSettingsQueue = xQueueCreate(1, sizeof(WifiSettings));

    taskInitializedGroup = xEventGroupCreate();
    mqttEventGroup = xEventGroupCreate();

    sendRelayStatusSemaphore = xSemaphoreCreateBinary();

    esp_task_wdt_deinit();
    esp_log_level_set("*", ESP_LOG_INFO);
    // start tasks
    xTaskCreate(debugTask, "Debug task", 4096, NULL, 5, NULL);
    xTaskCreate(relayTask, "Relay task", 4096, NULL, 5, NULL);
    xTaskCreate(mqttTask, "mqtt task", 4096, NULL, 5, NULL);
    xTaskCreate(internetTask, "internet task", 24576 + configMINIMAL_STACK_SIZE, NULL, 5, NULL);
}
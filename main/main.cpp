#include "main.h"

#include "tasks/debugTask.h"
#include "tasks/mqttTask.h"
#include "tasks/relayTask.h"

QueueHandle_t debugQueue;
QueueHandle_t mqttQueue;
QueueHandle_t relayQueue;
QueueHandle_t priceQueue;

extern "C" void app_main() {
    initArduino();

    debugQueue = xQueueCreate(10, sizeof(DebugMessage));
    mqttQueue = xQueueCreate(10, sizeof(mqttMessage));
    relayQueue = xQueueCreate(10, sizeof(RelaySettings));
    priceQueue = xQueueCreate(10, sizeof(double[6]));

    // start tasks
    xTaskCreate(relayTask, "Relay task", 4096, NULL, 5, NULL);
    xTaskCreate(debugTask, "Debug task", 4096, NULL, 5, NULL);
    xTaskCreate(mqttTask, "mqtt task", 4096, NULL, 5, NULL);
}
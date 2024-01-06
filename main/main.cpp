#include "main.h"

#include "tasks/debugTask.h"
#include "tasks/relayTask.h"

QueueHandle_t debugQueue;
extern "C" void app_main() {
    initArduino();

    debugQueue = xQueueCreate(10, sizeof(DebugMessage));
    // start tasks
    xTaskCreate(relayTask, "Relay task", 4096, NULL, 5, NULL);
    xTaskCreate(debugTask, "Debug task", 4096, NULL, 5, NULL);
}
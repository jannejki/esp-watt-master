#include "Arduino.h"
#include "tasks/debugTask.h"
#include "tasks/relayTask.h"


extern "C" void app_main() {
    initArduino();

    // start tasks
    xTaskCreate(relayTask, "Relay task", 4096, NULL, 5, NULL);
    xTaskCreate(debugTask, "Debug task", 4096, NULL, 5, NULL);
}
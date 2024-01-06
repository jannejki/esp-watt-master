#include "tasks/relayTask.h"

void relayTask(void* params) {
    pinMode(0, OUTPUT);

    DebugMessage debugMessage;
    while (1) {
        digitalWrite(0, HIGH);
        vTaskDelay(500);
        digitalWrite(0, LOW);
        vTaskDelay(500);
    }
}
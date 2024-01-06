#include "tasks/debugTask.h"

void debugTask(void* params) {
    Serial.begin(115200);
    while (1) {
        Serial.println("Hello World");
        vTaskDelay(1000);
    }
}
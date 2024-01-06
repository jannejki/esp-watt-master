#include "tasks/relayTask.h"

void relayTask(void* params) {
    pinMode(0, OUTPUT);
    pinMode(35, OUTPUT);

    while (1) {
        digitalWrite(0, HIGH);
        digitalWrite(35, HIGH);
        vTaskDelay(500);
        digitalWrite(0, LOW);
        digitalWrite(35, LOW);
        vTaskDelay(500);
    }
}
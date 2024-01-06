#include "tasks/relayTask.h"

void relayTask(void* params) {
    pinMode(0, OUTPUT);

    DebugMessage debugMessage;
    RelayMessage relayMessage;

    while (1) {
        if (xQueueReceive(relayQueue, &relayMessage, portMAX_DELAY) == pdTRUE) {
            Serial.print("Relay number: ");
            Serial.println(relayMessage.relayNumber);
            Serial.print("Relay state: ");
            Serial.println(relayMessage.state);
            Serial.print("Relay mode: ");
            Serial.println(relayMessage.mode);
            Serial.print("Relay threshold: ");
            Serial.println(relayMessage.threshold);
            digitalWrite(0, HIGH);
        }
    }
}
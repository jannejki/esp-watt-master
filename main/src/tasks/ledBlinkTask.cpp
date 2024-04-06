#include "tasks/ledBlinkTask.h"

void ledBlinkTask(void* params) {
    struct ledBlinkTaskParams settings = *(ledBlinkTaskParams*)params;
    pinMode(settings.pin, OUTPUT);

    while (1) {
        *settings.loopFinished = false;
        digitalWrite(settings.pin, HIGH);
        vTaskDelay(settings.delay / portTICK_PERIOD_MS);
        digitalWrite(settings.pin, LOW);
        *settings.loopFinished = true;
        vTaskDelay(settings.delay / portTICK_PERIOD_MS);
    }
}
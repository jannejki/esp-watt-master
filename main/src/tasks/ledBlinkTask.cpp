#include "tasks/ledBlinkTask.h"

void ledBlinkTask(void* params) {
    ESP_LOGI("ledBlinkTask", "ledBlinkTask started\n\r");
    struct ledBlinkTaskParams settings = *(ledBlinkTaskParams*)params;

    ESP_LOGI("ledBlinkTask", "LED: %d, delay: %d", settings.pin, settings.delay);
    /*
    struct ledBlinkTaskParams {
    uint8_t pin;
    int delay;
    bool *loopFinished;
};
    */
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
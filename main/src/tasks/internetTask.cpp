#include "tasks/internetTask.h"

void debug(char* message);
boolean waitForDebugTask();

void internetTask(void* params) {
    waitForDebugTask();
    Wifi wifi((uint8_t*)SECRET_WIFI_SSID, (uint8_t*)SECRET_WIFI_PASSWORD);

    wifi.init();
    while (1) {
    }
}

void debug(char* message) {
    DebugMessage debug;
    strncpy(debug.message, message, 63);
    debug.message[63] = '\0';  // Ensure null-termination
    debug.tick = xTaskGetTickCount();
    debug.sender = "internetTask";

    xQueueSend(debugQueue, &debug, 0);
}

boolean waitForDebugTask() {
    xEventGroupWaitBits(
        taskInitializedGroup, /* The event group being tested. */
        DEBUG_TASK,           /* The bits within the event group to wait for. */
        pdTRUE, /* DEBUG_TASK -bit should be cleared before returning. */
        pdTRUE, /* Don't wait for both bits, either bit will do. */
        portMAX_DELAY); /* Wait a maximum of 100ms for either bit to be set. */
    debug("internetTask started\n\r");

    return true;
}

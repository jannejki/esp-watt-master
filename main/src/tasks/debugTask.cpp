#include "tasks/debugTask.h"

#include "main.h"

bool LOG = true;

void debug(DebugMessage* debugMessage);

void debugTask(void* params) {
 
    Serial.begin(115200);
    DebugMessage debugMessage;
    String message;

    CommandInterface commandInterface(CONFIG_DEBUG_LED_PIN);

    EventBits_t uxBits;

    /* Set DEBUG_TASK BIT in xEventGroup. */
    uxBits = xEventGroupSetBits(
        taskInitializedGroup, /* The event group being updated. */
        DEBUG_TASK);          /* The bits being set. */
    Serial.println("debugTask started");
    while (1) {
        if (Serial.available()) {
            char c = Serial.read();
            Serial.print(c);

            if (c == '\r') {
                Serial.print('\n');
                commandInterface.commandEntered((char*)message.c_str());
                message = "";
            } else if (c == 127) {
                message.remove(message.length() - 1);
            } else {
                message += c;
            }
        }

        if (xQueueReceive(debugQueue, &debugMessage, 0) == pdTRUE) {
            debug(&debugMessage);
        }
        vTaskDelay(20);
    }
}

void debug(DebugMessage* debugMessage) {
    ////if (!LOG) return;

    Serial.print(debugMessage->sender);
    Serial.print(": ");
    Serial.print(debugMessage->message);
}
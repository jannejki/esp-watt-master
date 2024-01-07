#include "tasks/debugTask.h"

#include "main.h"

bool LOG = false;

void debug(DebugMessage* debugMessage);

void debugTask(void* params) {
    Serial.begin(115200);

    DebugMessage debugMessage;
    String message;

    CommandInterface commandInterface(LED1);
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
    if (!LOG) return;
    char* message = (char*)debugMessage->message.c_str();
    char* sender = (char*)debugMessage->sender.c_str();
    ESP_LOGI(sender, "%s", message);
}
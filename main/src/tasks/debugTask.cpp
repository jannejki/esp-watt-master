#include "tasks/debugTask.h"

#include "main.h"

void debugTask(void* params) {
    Serial.begin(115200);

    DebugMessage debugMessage;
    String message;

    CommandInterface commandInterface(35);
    while (1) {
        if (Serial.available()) {
            char c = Serial.read();
            Serial.print(c);

            if (c == '\r') {
                Serial.print('\n');
                commandInterface.commandEntered((char*)message.c_str());
                message = "";
            } else if(c == 127){
                message.remove(message.length() - 1);
            } else {
                message += c;
            }
        }

        if (xQueueReceive(debugQueue, &debugMessage, 0) == pdTRUE) {
            Serial.print(debugMessage.message);
            Serial.print(" ");
            Serial.println(debugMessage.tick);
        }
        vTaskDelay(20);
    }
}
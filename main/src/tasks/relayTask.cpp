#include "tasks/relayTask.h"

void relayTask(void* params) {
    Relay* relays = new Relay[3];

    relays[0].initialize(LED0, 0);
    relays[1].initialize(LED1, 1);

    DebugMessage debugMessage;
    RelaySettings relaySettings;

    while (1) {
        if (xQueueReceive(relayQueue, &relaySettings, portMAX_DELAY) == pdTRUE) {
            Relay::Mode mode = Relay::manual;
            bool state = false;

            switch (relaySettings.mode) {
                case automatic:
                    mode = Relay::automatic;
                    break;
                case manual:
                    mode = Relay::manual;
                    break;
                case noModeChange:
                    mode = relays[relaySettings.relayNumber].readMode();
            }

            switch (relaySettings.state) {
                case on:
                    state = true;
                    relaySettings.mode = manual;
                    mode = Relay::manual;
                    break;
                case off:
                    state = false;
                    relaySettings.mode = manual;
                    mode = Relay::manual;
                    break;
                case noStateChange:
                    state = relays[relaySettings.relayNumber].readState();
            }

            if (relaySettings.threshold != -1.0) {
                relays[relaySettings.relayNumber].updatePriceThreshold(
                    relaySettings.threshold);
            }

            relays[relaySettings.relayNumber].changeState(state);
            relays[relaySettings.relayNumber].changeMode(mode);
            
            // Sending status to debug queue
            String relayStatus = relays[relaySettings.relayNumber].status();
            Serial.println(relayStatus);
            debugMessage.message = relayStatus;
            debugMessage.sender = "relay task";
            xQueueSend(debugQueue, &debugMessage, 100);
        }
    }
}
#include "tasks/relayTask.h"

void updateElectricPrices(double *price, Relay *relays);

void relayTask(void *params) {
    Relay *relays = new Relay[3];

    relays[0].initialize(LED0, 0);
    relays[1].initialize(LED1, 1);

    DebugMessage debugMessage;
    RelaySettings relaySettings;
    double electricPrices[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

    while (1) {
        if (xQueueReceive(relayQueue, &relaySettings, 0) == pdTRUE) {
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

            sprintf(debugMessage.message, "Relay %d: %s",
                    relaySettings.relayNumber, relayStatus.c_str());

            debugMessage.sender = "relay task";
            xQueueSend(debugQueue, &debugMessage, 100);

        } else if (xQueueReceive(priceQueue, &(electricPrices), 100) ==
                   pdPASS) {
            Serial.println("price received");
            updateElectricPrices(electricPrices, relays);
        }

        vTaskDelay(50);
    }
}

void updateElectricPrices(double *price, Relay *relays) {
    for (int i = 0; i < sizeof(relays); i++) {  // Assuming you have 3 relays
        relays[i].updatePrice(price[0]);
        String status = relays[i].status();
        Serial.println(status);
    }
}
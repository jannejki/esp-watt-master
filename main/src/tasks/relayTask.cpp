#include "tasks/relayTask.h"

void updateElectricPrices(double* price, Relay* relays, uint8_t amountOfRelays);

void relayTask(void* params) {
    Relay* relays = new Relay[(uint8_t)CONFIG_AMOUNT_OF_RELAYS];

    relays[0].initialize(LED0, 0);
    relays[1].initialize(LED1, 1);

    DebugMessage debugMessage;
    RelaySettings relaySettings;
    double electricPrices[6] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

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
            char* relayStatus = relays[relaySettings.relayNumber].status();
            ESP_LOGI("Relay", "%s", relayStatus);
            mqttMessage mqttTransmitMessage;
            sprintf(mqttTransmitMessage.topic, "%s", MQTT_DEVICE_STATUS_TOPIC);
            sprintf(mqttTransmitMessage.message, "%s", relayStatus);

            xQueueSend(mqttTransmitQueue, &mqttTransmitMessage, 100);
        }
        else if (xQueueReceive(priceQueue, &(electricPrices), 100) ==
            pdPASS) {
            Serial.println("price received");
            updateElectricPrices(electricPrices, relays, CONFIG_AMOUNT_OF_RELAYS);
        }

        vTaskDelay(50);
    }
}

void updateElectricPrices(double* price, Relay* relays, uint8_t amountOfRelays) {
    mqttMessage mqttTransmitMessage;
    for (int i = 0; i < amountOfRelays; i++) {
        relays[i].updatePrice(price[0]);
        // Sending status to debug queue
        char* relayStatus = relays[i].status();
        ESP_LOGI("Relay", "%s", relayStatus);
        sprintf(mqttTransmitMessage.topic, "%s", MQTT_DEVICE_STATUS_TOPIC);
        sprintf(mqttTransmitMessage.message, "%s", relayStatus);

        xQueueSend(mqttTransmitQueue, &mqttTransmitMessage, 100);
    }
}
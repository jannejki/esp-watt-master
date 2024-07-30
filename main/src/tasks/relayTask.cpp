#include "tasks/relayTask.h"

void updateElectricPrices(double* price, Relay* relays, uint8_t amountOfRelays);
void sendUpdatedRelayToDisplay(Relay* relay);

void relayTask(void* params) {
    Relay* relays = new Relay[(uint8_t)CONFIG_AMOUNT_OF_RELAYS];

    relays[0].initialize(CONFIG_RELAY_0_PIN, 0);
    relays[1].initialize(CONFIG_RELAY_1_PIN, 1);

    DebugMessage debugMessage;
    RelaySettings relaySettings;

    double electricPrices[6] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
    sendUpdatedRelayToDisplay(&relays[0]);
    sendUpdatedRelayToDisplay(&relays[1]);

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

            if (relaySettings.price != NULL) {
                relays[relaySettings.relayNumber].updatePrice(relaySettings.price);
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
            sendUpdatedRelayToDisplay(&relays[relaySettings.relayNumber]);


        }
        else if (xQueueReceive(priceQueue, &(electricPrices), 100) ==
            pdPASS) {
            Serial.println("price received");
            updateElectricPrices(electricPrices, relays, CONFIG_AMOUNT_OF_RELAYS);
        }

        if (xSemaphoreTake(sendRelayStatusSemaphore, (TickType_t)0) == pdTRUE) {
            for (int i = 0; i < CONFIG_AMOUNT_OF_RELAYS; i++) {
                char* relayStatus = relays[i].status();
                ESP_LOGI("Relay", "%s", relayStatus);
                mqttMessage mqttTransmitMessage;
                sprintf(mqttTransmitMessage.topic, "%s", MQTT_DEVICE_STATUS_TOPIC);
                sprintf(mqttTransmitMessage.message, "%s", relayStatus);
                xQueueSend(mqttTransmitQueue, &mqttTransmitMessage, 100);
            }
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

        sendUpdatedRelayToDisplay(&relays[i]);
    }
}

void sendUpdatedRelayToDisplay(Relay* relay) {
    DisplayMessage displayMessage;
    displayMessage.updateType = RELAY_UPDATE;

    relayState state = relay->readState() ? on : off;
    relayMode mode = relay->readMode() == automatic ? relayMode::automatic : relayMode::manual;
    displayMessage.relay.relayNumber = relay->relayNumber;
    displayMessage.relay.state = state;
    displayMessage.relay.mode = mode;
    displayMessage.relay.threshold = relay->priceThreshold;

    xQueueSend(displayQueue, &displayMessage, 100);
}
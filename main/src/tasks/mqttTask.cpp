#include "tasks/mqttTask.h"

std::vector<String> splitMQTTMessageToCommands(String input);
std::vector<String> splitMQTTCommandsToParams(String input);

RelaySettings parseMqttRelaySettings(std::vector<String> tokens);

/*
void parseMqttElectricPriceMessage(std::vector<std::string> tokens,
double *prices);*/

void mqttTask(void *params) {
    mqttMessage mqtt;
    DebugMessage debugMessage;
    debugMessage.sender = "mqttTask";

    while (1) {
        if (xQueueReceive(mqttQueue, &mqtt, portMAX_DELAY)) {
            debugMessage.tick = xTaskGetTickCount();
            xQueueSend(debugQueue, &debugMessage, 0);

            std::vector<String> commands =
                splitMQTTMessageToCommands(mqtt.message);

            if (mqtt.topic == "relay") {
                RelaySettings relay = parseMqttRelaySettings(commands);

                if (relay.relayNumber == -1) {
                    debugMessage.message = "relay number is not set!";
                    xQueueSend(debugQueue, &debugMessage, (TickType_t)100);
                    Serial.print("relay number is not set: ");
                    Serial.println(relay.relayNumber);
                } else {
                    xQueueSend(relayQueue, &relay, (TickType_t)100);
                }

            } else if (mqtt.topic == "electric/price") {
                Serial.println("topic: electric/price");
            } else {
                Serial.println("topic: ei tunneta");
            }
        }
    }
}

std::vector<String> splitMQTTMessageToCommands(String input) {
    std::vector<String> tokens;

    int pos = 0;
    while ((pos = input.indexOf('&')) != -1) {
        tokens.push_back(input.substring(0, pos));
        input.remove(0, pos + 1);
    }

    // Add the last token
    tokens.push_back(input);

    return tokens;
}

std::vector<String> splitMQTTCommandsToParams(String input) {
    std::vector<String> tokens;

    int pos = 0;
    while ((pos = input.indexOf('=')) != -1) {
        tokens.push_back(input.substring(0, pos));
        input.remove(0, pos + 1);
    }

    // Add the last token
    tokens.push_back(input);

    return tokens;
}

struct RelaySettings parseMqttRelaySettings(std::vector<String> tokens) {
    int relayNumber = -1;
    enum relayMode mode = noModeChange;
    enum relayState state = noStateChange;
    double threshold = -1;

    DebugMessage debug;
    debug.sender = "mqttTask ParseMqttRelaySettings";
    debug.tick = xTaskGetTickCount();

    RelaySettings relaySettings;

    for (size_t i = 0; i < tokens.size(); ++i) {
        std::vector<String> params = splitMQTTCommandsToParams(tokens[i]);
        if (params[0] == "relay") {
            // convert params[1] to int
            relayNumber = params[1].toInt();

        } else if (params[0] == "mode") {
            if (params[1] == "auto") {
                mode = automatic;
            } else if (params[1] == "manual") {
                mode = manual;
            } else {
                debug.message = "mode='" + params[1] + "' is not acceptable!";
                xQueueSend(debugQueue, &debug, (TickType_t)100);
            }

        } else if (params[0] == "state") {
            if (params[1] == "on") {
                state = on;
            } else if (params[1] == "off") {
                state = off;
            } else {
                debug.message = "state='" + params[1] + "' is not acceptable!";
                xQueueSend(debugQueue, &debug, (TickType_t)100);
            }

        } else if (params[0] == "threshold") {
            threshold = params[1].toDouble();

        } else {
            debug.message = "Unknown command: '" + params[0] + "'.";
            xQueueSend(debugQueue, &debug, (TickType_t)100);
        }
    }

    relaySettings.relayNumber = relayNumber;
    relaySettings.state = state;
    relaySettings.mode = mode;
    relaySettings.threshold = threshold;

    return relaySettings;
}
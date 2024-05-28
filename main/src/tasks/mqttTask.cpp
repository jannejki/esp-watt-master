#include "tasks/mqttTask.h"

std::vector<String> splitMQTTMessageToCommands(char* input);
std::vector<String> splitMQTTCommandsToParams(String input);

RelaySettings parseMqttRelaySettings(std::vector<String> tokens);

void parseMqttElectricPriceMessage(std::vector<String> tokens, double* prices);

void mqttTask(void* params) {
    mqttMessage mqtt;
    DebugMessage debugMessage;
    debugMessage.sender = "mqttTask";

    while (1) {
        if (xQueueReceive(mqttReceiveQueue, &mqtt, portMAX_DELAY)) {
            std::vector<String> commands = splitMQTTMessageToCommands(mqtt.message);

            // If the message is a ping message, send the status of the relays
            if(strcmp(mqtt.topic, MQTT_DEVICE_PING_TOPIC) == 0) {
                if(strcmp(mqtt.message, "status") == 0) {
                    xSemaphoreGive(sendRelayStatusSemaphore);
                }
                else {
                    ESP_LOGE("mqttTask", "Received unknown message: '%s' to topic: '%s'", mqtt.message, mqtt.topic);
                }
            }


            if (strcmp(mqtt.topic, MQTT_DEVICE_COMMAND_TOPIC) == 0) {
                RelaySettings relay = parseMqttRelaySettings(commands);

                if (relay.relayNumber == -1) {
                    ESP_LOGE("mqttTask", "Relay number not found!");
                }
                else {
                    xQueueSend(relayQueue, &relay, (TickType_t)100);
                }
            }
            else if (strcmp(mqtt.topic, "electric/price") == 0) {
                double prices[6] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
                parseMqttElectricPriceMessage(commands, prices);
                xQueueSend(priceQueue, &prices, (TickType_t)100);
            }
            else {
                Serial.println("topic: ei tunneta");
            }

        }
    }
}

std::vector<String> splitMQTTMessageToCommands(char* rawInput) {
    std::vector<String> tokens;
    // convert char* to string
    String input = String(rawInput);
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
    double price = NULL;

    DebugMessage debug;
    char buffer[64];
    debug.sender = "mqttTask ParseMqttRelaySettings";
    debug.tick = xTaskGetTickCount();

    RelaySettings relaySettings;

    for (size_t i = 0; i < tokens.size(); ++i) {
        std::vector<String> params = splitMQTTCommandsToParams(tokens[i]);
        if (params[0] == "relay") {
            // convert params[1] to int
            relayNumber = params[1].toInt();

        }
        else if (params[0] == "mode") {
            if (params[1] == "auto") {
                mode = automatic;
            }
            else if (params[1] == "manual") {
                mode = manual;
            }
            else {

                sprintf(debug.message, "mode='%s' is not acceptable!",
                    params[1].c_str());
                xQueueSend(debugQueue, &debug, (TickType_t)100);
            }

        }

        else if (params[0] == "state") {
            if (params[1] == "on") {
                state = on;
            }
            else if (params[1] == "off") {
                state = off;
            }
            else {
                sprintf(debug.message, "state='%s' is not acceptable!",
                    params[1].c_str());

                xQueueSend(debugQueue, &debug, (TickType_t)100);
            }

        }

        else if (params[0] == "threshold") {
            threshold = params[1].toDouble();
        }

        else if (params[0] == "price") {
            price = params[1].toDouble();
        }

        else {
            sprintf(debug.message, "Unknown command: '%s'.", params[0].c_str());

            xQueueSend(debugQueue, &debug, (TickType_t)100);
        }
    }

    relaySettings.relayNumber = relayNumber;
    relaySettings.state = state;
    relaySettings.mode = mode;
    relaySettings.threshold = threshold;
    relaySettings.price = price;

    return relaySettings;
}

void parseMqttElectricPriceMessage(std::vector<String> tokens, double* prices) {
    DebugMessage debug;

    for (size_t i = 0; i < tokens.size(); ++i) {
        std::vector<String> params = splitMQTTCommandsToParams(tokens[i]);
        sprintf(debug.message, "params[0] = %s, params[1] = %s",
            params[0].c_str(), params[1].c_str());
        xQueueSend(debugQueue, &debug, (TickType_t)100);
        if (params[0] == "hour0") {
            prices[0] = params[1].toDouble();
        }
        else if (params[0] == "hour1") {
            prices[1] = params[1].toDouble();
        }
        else if (params[0] == "hour2") {
            prices[2] = params[1].toDouble();
        }
        else if (params[0] == "hour3") {
            prices[3] = params[1].toDouble();
        }
        else if (params[0] == "hour4") {
            prices[4] = params[1].toDouble();
        }
        else if (params[0] == "hour5") {
            prices[5] = params[1].toDouble();
        }
    }
}

#include "objects/CommandInterface.h"

CommandInterface::CommandInterface(uint8_t _ledPin) : ledPin(_ledPin) {
    populateCommandMap();
    pinMode(ledPin, OUTPUT);
}

CommandInterface::~CommandInterface() {}

void CommandInterface::ledHandler(String input) {
    std::vector<String> tokens = splitCommandsAndArgs(input);

    if (tokens.size() > 1) {
        if (tokens[1] == "on") {
            digitalWrite(ledPin, HIGH);
        } else if (tokens[1] == "off") {
            digitalWrite(ledPin, LOW);
        } else {
            Serial.println(
                "Wrong parameter! 'help led' to get instructions. Inserted "
                "parameter: '" +
                tokens[1] + "'.");
        }
    } else {
        if (digitalRead(ledPin) == HIGH) {
            Serial.println("Led is on");
        } else {
            Serial.println("Led is off");
        }
    }
}

void CommandInterface::tickHandler(String input) {}

void CommandInterface::commandPrinter(String input) {
    std::vector<String> tokens = splitCommandsAndArgs(input);

    if (tokens.size() > 1) {
        Serial.println(commandMap[tokens[1]].info.c_str());
        Serial.println(commandMap[tokens[1]].commands.c_str());
    } else {
        String text;
        for (const auto &item : commandMap) {
            text = String(item.second.name.c_str()) + " - " +
                   String(item.second.info.c_str());
            Serial.println(text.c_str());
        }
    }
}

void CommandInterface::runtimePrinter(String input) {}

void CommandInterface::populateCommandMap() {
    // Use lambda functions to adapt member functions to std::function signature
    commandMap["tick"] =
        CommandFunction{"tick", [this](String input) { tickHandler(input); },
                        "Prints current tick count", "No commands"};

    commandMap["help"] = CommandFunction{
        "help", [this](String input) { commandPrinter(input); },
        "Prints commands and info",
        "write help [COMMAND] to get valid commands and parameters"};

    commandMap["led"] =
        CommandFunction{"led", [this](String input) { ledHandler(input); },
                        "Toggles LED on or off.", " Use 'led on' or 'led off'"};

    commandMap["runtime"] = CommandFunction{
        "runtime", [this](String input) { runtimePrinter(input); },
        "Prints freeRTOS runtime stats"};

    commandMap["AT"] =
        CommandFunction{"AT", [this](String input) { sendMessageToEsp(input); },
                        "Sends AT messages to ESP."};

    commandMap["relay"] = CommandFunction{
        "relay", [this](String input) { relayHandler(input); },
        "Read and control relays.",
        "use 'relay [relay number] [on/off]' to toggle relays. use 'relay "
        "[relay number]' to get relay's info."};

    commandMap["mqtt"] = CommandFunction{
        "mqtt", [this](String input) { simulateMQTTMessages(input); },
        "Simulating MQTT messaging",
        "use 'mqtt [topic] [message]' to simulate sending mqtt messages."};

    commandMap["restart"] =
        CommandFunction{"restart", [this](String input) { restart(input); },
                        "Restarts the ESP", "Restarts the ESP."};
}

void CommandInterface::sendMessageToEsp(String input) {}

void CommandInterface::restart(String input) {
    Serial.println("Restarting...");
    ESP.restart();
}

void CommandInterface::simulateMQTTMessages(String input) {
    mqttMessage mqtt;

    // Find the positions of the first and second spaces
    int firstSpace = input.indexOf(" ");
    int secondSpace = input.indexOf(" ", firstSpace + 1);

    if (firstSpace == -1 || secondSpace == -1) {
        return;
    }

    // Extract the topic and message
    String topic = input.substring(firstSpace + 1, secondSpace);
    String message = input.substring(secondSpace + 1);

    mqtt.message = message;
    mqtt.topic = topic;

    xQueueSend(mqttQueue, &mqtt, (TickType_t)10);
}

void CommandInterface::relayHandler(String input) {}

void CommandInterface::commandEntered(String input) {
    if (input.length() == 0) return;
    DebugMessage debugMessage;
    debugMessage.sender = "command interface";
    // Create a copy of the input
    String inputCopy = input;

    // get the first word from the String
    String command = inputCopy.substring(0, inputCopy.indexOf(" "));

    // Trim the command string
    command.trim();

    if (commandMap.find(command) != commandMap.end()) {
        debugMessage.tick = xTaskGetTickCount();
        sprintf(debugMessage.message, "found command: '%s'", command.c_str());

        xQueueSend(debugQueue, &debugMessage, 0);
        // Call the handling function for the received command
        commandMap[command].func(inputCopy);
    } else {
        debugMessage.tick = xTaskGetTickCount();
        sprintf(debugMessage.message, "Unknown command: '%s'", command.c_str());

        xQueueSend(debugQueue, &debugMessage, 0);
        // Handle unknown command
    }
}

std::vector<String> CommandInterface::splitCommandsAndArgs(String input) {
    std::vector<String> tokens;

    int pos = 0;
    while ((pos = input.indexOf(' ')) != -1) {
        tokens.push_back(input.substring(0, pos));
        input.remove(0, pos + 1);
    }

    // Add the last token
    tokens.push_back(input);

    return tokens;
}

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
        }
        else if (tokens[1] == "off") {
            digitalWrite(ledPin, LOW);
        }
        else {
            Serial.println(
                "Wrong parameter! 'help led' to get instructions. Inserted "
                "parameter: '" +
                tokens[1] + "'.");
        }
    }
    else {
        if (digitalRead(ledPin) == HIGH) {
            Serial.println("Led is on");
        }
        else {
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
    }
    else {
        String text;
        for (const auto& item : commandMap) {
            text = String(item.second.name.c_str()) + " - " +
                String(item.second.info.c_str());
            Serial.println(text.c_str());
        }
    }
}

void CommandInterface::runtimePrinter(String input) {}

void CommandInterface::changeDebugLevel(String input) {
    std::vector<String> tokens = splitCommandsAndArgs(input);

    if (tokens.size() > 1) {
        if (tokens[1] == "ERROR") {
            esp_log_level_set("*", ESP_LOG_ERROR);
            Serial.println("Debug level set to ERROR.");
        }
        else if (tokens[1] == "WARNING") {
            esp_log_level_set("*", ESP_LOG_WARN);
            Serial.println("Debug level set to WARNING.");
        }
        else if (tokens[1] == "INFO") {
            esp_log_level_set("*", ESP_LOG_INFO);
            Serial.println("Debug level set to INFO.");
        }
        else if (tokens[1] == "DEBUG") {
            esp_log_level_set("*", ESP_LOG_DEBUG);
            Serial.println("Debug level set to DEBUG.");
        }
        else if (tokens[1] == "VERBOSE") {
            esp_log_level_set("*", ESP_LOG_VERBOSE);
            Serial.println("Debug level set to VERBOSE.");
        }
        else if (tokens[1] == "NONE") {
            esp_log_level_set("*", ESP_LOG_NONE);
            Serial.println("Debug level set to NONE. No logs will be printed.");
        }
        else {
            Serial.println("Unknown debug level. Use 'help debug' to get valid debug levels.");
        }
    }
}

void CommandInterface::populateCommandMap() {
    // Use lambda functions to adapt member functions to std::function signature
    commandMap["tick"] =
        CommandFunction{ "tick", [this](String input) { tickHandler(input); },
                        "Prints current tick count", "No commands" };

    commandMap["help"] = CommandFunction{
        "help", [this](String input) { commandPrinter(input); },
        "Prints commands and info",
        "write help [COMMAND] to get valid commands and parameters" };

    commandMap["led"] =
        CommandFunction{ "led", [this](String input) { ledHandler(input); },
                        "Toggles LED on or off.", " Use 'led on' or 'led off'" };

    commandMap["runtime"] = CommandFunction{
        "runtime", [this](String input) { runtimePrinter(input); },
        "Prints freeRTOS runtime stats" };

    commandMap["AT"] =
        CommandFunction{ "AT", [this](String input) { sendMessageToEsp(input); },
                        "Sends AT messages to ESP." };

    commandMap["relay"] = CommandFunction{
        "relay", [this](String input) { relayHandler(input); },
        "Read and control relays.",
        "use 'relay [relay number] [on/off/auto/manual]' to change relay settings. use 'relay "
        "[relay number]' to get relay's info." };

    commandMap["mqtt"] = CommandFunction{
        "mqtt", [this](String input) { simulateMQTTMessages(input); },
        "Simulating MQTT messaging",
        "use 'mqtt [topic] [message]' to simulate sending mqtt messages." };

    commandMap["restart"] =
        CommandFunction{ "restart", [this](String input) { restart(input); },
                        "Restarts the ESP", "Restarts the ESP." };

    commandMap["debug"] = CommandFunction{
        "debug", [this](String input) { changeDebugLevel(input); },
        "Change debug level",
        "use 'debug [level]' to change debug level. Affects only for ESP's own logging system. Levels: ERROR, WARNING, INFO, DEBUG, VERBOSE, NONE." };
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
#if 0
    String message = input.substring(secondSpace + 1);

    mqtt.message = message;
    mqtt.topic = topic;

    xQueueSend(mqttReceiveQueue, &mqtt, (TickType_t)10);
#endif
}

void CommandInterface::relayHandler(String input) {
    if (input.length() == 0) return;
    std::vector<String> tokens = splitCommandsAndArgs(input);
    
    if (tokens.size() == 3) {
        char* end;
        long int relayNumber = strtol(tokens[1].c_str(), &end, 10);
        if (*end != '\0') {
            ESP_LOGI("relay handler", "%s is not a valid relay number", tokens[1].c_str());
            return;
        }

        RelaySettings relaySettings;
        relaySettings.relayNumber = relayNumber;

        if (tokens[2].compareTo("on") == 0) {
            relaySettings.state = on;
        }
        else if (tokens[2].compareTo("off") == 0) {
            relaySettings.state = off;
        }
        else if (tokens[2].compareTo("auto") == 0) {
            relaySettings.mode = automatic;
        }
        else if (tokens[2].compareTo("manual") == 0) {
            relaySettings.mode = manual;
        }
        else {
            ESP_LOGE("relay handler", "'%s' is not a valid setting!", tokens[2].c_str());
        }


        xQueueSend(relayQueue, &relaySettings, 100);
    }

    else if (tokens.size() == 2) {
        char* end;
        long int relayNumber = strtol(tokens[1].c_str(), &end, 10);
        if (*end != '\0') {
            ESP_LOGI("relay handler", "%s is not a valid relay number", tokens[1].c_str());
            return;
        }

        // Sending a "no change" command to the relayqueue to get the relay log out it's state etc without changing anyting
        RelaySettings relaySettings;
        relaySettings.relayNumber = relayNumber;
        relaySettings.mode = noModeChange;
        relaySettings.state = noStateChange;
        xQueueSend(relayQueue, &relaySettings, 100);

    }

}

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
        // Call the handling function for the received command
        commandMap[command].func(inputCopy);
    }
    else {
        debugMessage.tick = xTaskGetTickCount();
        sprintf(debugMessage.message, "Unknown command: '%s'\n\r", command.c_str());

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

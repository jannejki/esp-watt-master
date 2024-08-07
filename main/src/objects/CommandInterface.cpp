#include "objects/CommandInterface.h"
#define ARRAY_SIZE_OFFSET   5   // Increase this if print_real_time_stats returns ESP_ERR_INVALID_SIZE
static esp_err_t print_real_time_stats(TickType_t xTicksToWait)
{
    TaskStatus_t* start_array = NULL;
    TaskStatus_t* end_array = NULL;
    UBaseType_t start_array_size, end_array_size;
    uint32_t start_run_time, end_run_time;
    esp_err_t ret = ESP_OK; // Default return value

    // Allocate array to store current task states
    start_array_size = uxTaskGetNumberOfTasks() + ARRAY_SIZE_OFFSET;
    start_array = (TaskStatus_t*)malloc(sizeof(TaskStatus_t) * start_array_size);
    if (start_array == NULL) {
        return ESP_ERR_NO_MEM;
    }

    // Get current task states
    start_array_size = uxTaskGetSystemState(start_array, start_array_size, &start_run_time);
    if (start_array_size == 0) {
        ret = ESP_ERR_INVALID_SIZE;
    } else {
        vTaskDelay(xTicksToWait);

        // Allocate array to store tasks states post delay
        end_array_size = uxTaskGetNumberOfTasks() + ARRAY_SIZE_OFFSET;
        end_array = (TaskStatus_t*)malloc(sizeof(TaskStatus_t) * end_array_size);
        if (end_array == NULL) {
            ret = ESP_ERR_NO_MEM;
        } else {
            // Get post delay task states
            end_array_size = uxTaskGetSystemState(end_array, end_array_size, &end_run_time);
            if (end_array_size == 0) {
                ret = ESP_ERR_INVALID_SIZE;
            } else {
                // Calculate total_elapsed_time in units of run time stats clock period
                uint32_t total_elapsed_time = (end_run_time - start_run_time);
                if (total_elapsed_time == 0) {
                    ret = ESP_ERR_INVALID_STATE;
                } else {
                    printf("| Task | Run Time | Percentage | Min Stack Remaining\n");
                    // Match each task in start_array to those in the end_array
                    for (int i = 0; i < start_array_size; i++) {
                        int k = -1;
                        for (int j = 0; j < end_array_size; j++) {
                            if (start_array[i].xHandle == end_array[j].xHandle) {
                                k = j;
                                // Mark that task has been matched by overwriting their handles
                                start_array[i].xHandle = NULL;
                                end_array[j].xHandle = NULL;
                                break;
                            }
                        }
                        // Check if matching task found
                        if (k >= 0) {
                            uint32_t task_elapsed_time = end_array[k].ulRunTimeCounter - start_array[i].ulRunTimeCounter;
                            uint32_t percentage_time = (task_elapsed_time * 100UL) / (total_elapsed_time * portNUM_PROCESSORS);
                            UBaseType_t min_stack_remaining = uxTaskGetStackHighWaterMark(end_array[k].xHandle);
                            printf("| %s | %"PRIu32" | %"PRIu32"%% | %u\n", start_array[i].pcTaskName, task_elapsed_time, percentage_time, min_stack_remaining);
                        }
                    }

                    // Print unmatched tasks
                    for (int i = 0; i < start_array_size; i++) {
                        if (start_array[i].xHandle != NULL) {
                            printf("| %s | Deleted | N/A\n", start_array[i].pcTaskName);
                        }
                    }
                    for (int i = 0; i < end_array_size; i++) {
                        if (end_array[i].xHandle != NULL) {
                            UBaseType_t min_stack_remaining = uxTaskGetStackHighWaterMark(end_array[i].xHandle);
                            printf("| %s | Created | N/A | %u\n", end_array[i].pcTaskName, min_stack_remaining);
                        }
                    }
                }
            }
        }
    }

    // Free allocated memory
    free(start_array);
    free(end_array);
    return ret;
}

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

void CommandInterface::runtimePrinter(String input) {
    if (print_real_time_stats(pdMS_TO_TICKS(1000)) == ESP_OK) {
        printf("Real time stats obtained\n");
    }
    else {
        printf("Error getting real time stats\n");
    }
}

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

    // Find the positions of the first and second spaces
    int firstSpace = input.indexOf(" ");
    int secondSpace = input.indexOf(" ", firstSpace + 1);

    if (firstSpace == -1 || secondSpace == -1) {
        return;
    }

    // Extract the topic and message
    String topic = input.substring(firstSpace + 1, secondSpace);
#if 0
    mqttMessage mqtt;
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
        relaySettings.state = noStateChange;
        relaySettings.mode = noModeChange;

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

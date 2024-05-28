#ifndef COMMAND_INTERFACE_H_
#define COMMAND_INTERFACE_H_

#include <map>
#include <vector>

#include "../main.h"

class CommandInterface {
    using CommandHandler = std::function<void(String input)>;
    uint8_t ledPin;

    struct CommandFunction {
        String name;
        CommandHandler func;
        String info = "";
        String commands = "";
    };

    std::map<String, CommandFunction> commandMap;

   public:
    CommandInterface(uint8_t ledPin);
    virtual ~CommandInterface();

    void ledHandler(String input);
    void tickHandler(String input);
    void commandPrinter(String input);
    void runtimePrinter(String input);
    void populateCommandMap();
    void sendMessageToEsp(String input);
    void simulateMQTTMessages(String input);
    void relayHandler(String input);
    void restart(String input);
    void commandEntered(String input);
    void changeDebugLevel(String input);
    std::vector<String> splitCommandsAndArgs(String input);
};
#endif  // COMMAND_INTERFACE_H_
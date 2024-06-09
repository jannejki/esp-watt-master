#include "../main.h"

class Relay {
    int pin;

    bool state = false;
    double price;

    TickType_t lastToggle;
    TickType_t cooldownTicks = 500;

public:
    double priceThreshold;
    int relayNumber;

    enum Mode { automatic, manual };
    Mode mode = manual;

    Relay();
    virtual ~Relay();
    void initialize(int pin, int relayNumber);

    char* status();

    // price functions
    void updatePriceThreshold(double threshold);
    void updatePrice(double newPrice);

    // state functions
    void changeState(bool state);
    bool readState();

    // Mode functions
    void changeMode(enum Relay::Mode mode);
    enum Mode readMode();
};
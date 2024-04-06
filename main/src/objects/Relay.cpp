#include "objects/Relay.h"

Relay::Relay() {}

Relay::~Relay() {}

void Relay::initialize(int pin, int relayNumber) {
    this->pin = pin;
    this->relayNumber = relayNumber;
    this->mode = manual;
    this->state = false;
    this->priceThreshold = 0.0;
    this->price = 0.0;
    pinMode(pin, OUTPUT);
}

char* Relay::status() {
    String status = "Relay=" + String(relayNumber) + "&mode=";
    switch (mode) {
        case automatic:
            status += "auto";
            break;
        case manual:
            status += "manual";
            break;
    }

    status += "&state=";
    if (readState()) {
        status += "on";
    } else {
        status += "off";
    }

    status += "&price=" + String(price);

    status += "&priceThreshold=" + String(priceThreshold);

    char* cstr = new char[status.length() + 1];
    strcpy(cstr, status.c_str());

    return cstr;
}


/***************************************************/
/**************** price functions ******************/
/***************************************************/
void Relay::updatePriceThreshold(double threshold) {
    this->priceThreshold = threshold;

    if(this->mode == automatic) {
        changeState(this->price < this->priceThreshold);
    }
}

void Relay::updatePrice(double newPrice) {
    this->price = newPrice;

    if (this->mode == automatic) {
        changeState(this->price < this->priceThreshold);
    }
}


/***************************************************/
/**************** State functions ******************/
/***************************************************/
void Relay::changeState(bool state) {
    digitalWrite(pin, state);
}


bool Relay::readState() { return digitalRead(pin); }


/***************************************************/
/***************** mode functions ******************/
/***************************************************/
void Relay::changeMode(enum Relay::Mode mode) {
    this->mode = mode;
    if(mode == automatic) {
        changeState(this->price < this->priceThreshold);
    }
}

enum Relay::Mode Relay::readMode() { return this->mode; }
#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>

struct DebugMessage {
	char message[64];
	TickType_t tick;
};


extern QueueHandle_t debugQueue;

#endif  // MAIN_H
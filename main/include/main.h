#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#define LED0 0
#define LED1 35

struct DebugMessage {
	char message[64];
	TickType_t tick;
};


extern QueueHandle_t debugQueue;

#endif  // MAIN_H
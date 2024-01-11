#ifndef INTERNET_TASK_H
#define INTERNET_TASK_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <Webserver.h>
#include <WiFi.h>
#include "../main.h"
#include "Preferences.h"

void internetTask(void* params);

#endif  // INTERNET_TASK_H
#ifndef INTERNET_TASK_H
#define INTERNET_TASK_H

#include <Arduino.h>

#include <objects/Wifi.h>

#include "../main.h"
#include "../secrets.h"
#include "Preferences.h"


void internetTask(void* params);

#endif  // INTERNET_TASK_H
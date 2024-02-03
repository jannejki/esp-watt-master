#include "utils/debug.h"

void debug(const char* format, ...) {
    char msg[256];
    va_list args;
    va_start(args, format);
    vsprintf(msg, format, args);
    va_end(args);

    DebugMessage debug;
    debug.sender = "Wifi";

    strncpy(debug.message, msg, 63);
    xQueueSend(debugQueue, &debug, 0);
}
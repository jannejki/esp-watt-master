#include "tasks/internetTask.h"

boolean waitForDebugTask();


//=======================================================================
//======================== WiFI station =================================
//=======================================================================

Preferences wifiSettings;

void internetTask(void* params) {
    waitForDebugTask();
    TaskHandle_t ledTask = NULL;
    wifiSettings.begin("wifi", false);
    uint8_t mqttLedPin = 4;
    uint8_t wifiLedPin = 5;
    pinMode(wifiLedPin, OUTPUT);
    pinMode(mqttLedPin, OUTPUT);
    
    String ssid = wifiSettings.getString(WIFI_SETTINGS_SSID_KEY);
    String password = wifiSettings.getString(WIFI_SETTINGS_PASSWORD_KEY);
    Wifi wifi;

    bool ledTaskFinished = false;
    int delay = 1000;
    struct ledBlinkTaskParams wifiLed = { wifiLedPin, delay, &ledTaskFinished };
    struct ledBlinkTaskParams mqttLed = { mqttLedPin, delay, &ledTaskFinished };


    vTaskDelay(2000);
    if (ledTask == NULL) {
        xTaskCreate(
            ledBlinkTask,       /* Function that implements the task. */
            "WIFI_LED",          /* Text name for the task. */
            4096,      /* Stack size in words, not bytes. */
            (void*)&wifiLed,    /* Parameter passed into the task. */
            tskIDLE_PRIORITY,/* Priority at which the task is created. */
            &ledTask);      /* Used to pass out the created task's handle. */
    }


    wifi.mode(WIFI_MODE_STA);

    WifiState wifiState = wifi.connectToWifi(ssid, password);
    while (!ledTaskFinished) {/*Wait for ledTask to finish*/ }
    vTaskDelete(ledTask);
    ledTask = NULL;
    WifiSettings settings;
    if (wifiState == DISCONNECTED) {
        ESP_LOGE("internetTask", "Failed to connect to WiFi");
        wifi.mode(WIFI_MODE_APSTA);
        digitalWrite(wifiLedPin, HIGH);

        while (wifiState == DISCONNECTED) {
            wifiState = wifi.getWifiState(&settings);
            ESP_LOGI("internetTask", "Wifi state: %d", wifiState);

            vTaskDelay(1000);
        }
    }

    ESP_LOGI("internetTask", "Connected to WiFi");
    if (ledTask == NULL) {
        xTaskCreate(ledBlinkTask,
            "MQTT_LED",
            4096,
            (void*)&mqttLed,
            tskIDLE_PRIORITY,
            &ledTask);
    }

    while (1) {

        vTaskDelay(1000);
    }
}

boolean waitForDebugTask() {
    xEventGroupWaitBits(
        taskInitializedGroup, /* The event group being tested. */
        DEBUG_TASK,           /* The bits within the event group to wait for. */
        pdTRUE, /* DEBUG_TASK -bit should be cleared before returning. */
        pdTRUE, /* Don't wait for both bits, either bit will do. */
        portMAX_DELAY); /* Wait a maximum of 100ms for either bit to be set.
                         */
    debug("internetTask started\n\r");

    return true;
}
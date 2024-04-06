#include "tasks/internetTask.h"
#define GREEN_LED_PIN 4
#define YELLOW_LED_PIN 5
#define AP_BUTTON 6
//=======================================================================
//===================== Function definitions ============================
//=======================================================================
boolean waitForDebugTask();
void createAP(Wifi* wifi);
void stopAP(Wifi* wifi);

//=======================================================================
//======================== Global variables =============================
//=======================================================================
TaskHandle_t ledTask;
struct ledBlinkTaskParams ledParams;
bool ledTaskFinished;
WifiState wifiState;
Preferences savedSettings;

//=======================================================================
//======================== Internet task ================================
//=======================================================================
void internetTask(void* params) {
    waitForDebugTask();

    pinMode(AP_BUTTON, INPUT);
    uint8_t greenLed = GREEN_LED_PIN;
    uint8_t yellowLed = YELLOW_LED_PIN;

    pinMode(greenLed, OUTPUT);
    pinMode(yellowLed, OUTPUT);
    boolean apButtonPressed = digitalRead(AP_BUTTON);
    TaskHandle_t ledTask = NULL;
    savedSettings.begin("wifi", false);

    String ssid = savedSettings.getString(WIFI_SETTINGS_SSID_KEY);
    String password = savedSettings.getString(WIFI_SETTINGS_PASSWORD_KEY);

    Wifi wifi;
    wifiState = DISCONNECTED;
    ledTaskFinished = false;
    ledParams.pin = yellowLed;
    ledParams.delay = 1000;
    ledParams.loopFinished = &ledTaskFinished;

    WifiSettings settings;

    EventBits_t mqttFlags;

    if (!apButtonPressed) {
        if (ledTask == NULL) {
            xTaskCreate(
                ledBlinkTask,       /* Function that implements the task. */
                "WIFI_LED",          /* Text name for the task. */
                4096,      /* Stack size in words, not bytes. */
                (void*)&ledParams,    /* Parameter passed into the task. */
                tskIDLE_PRIORITY,/* Priority at which the task is created. */
                &ledTask);      /* Used to pass out the created task's handle. */
        }

        wifi.mode(WIFI_MODE_STA);
        wifiState = wifi.connectToWifi(ssid, password);
        while (!ledTaskFinished) {/*Wait for ledTask to finish, ledTaskFinished is toggled in the led task*/ }
        vTaskDelete(ledTask);
        ledTask = NULL;
    }

    // If couldn't connect to wifi create AP and wait for settings
    if (wifiState == DISCONNECTED) {
        createAP(&wifi);

        //-----------------------------------------------------------------------
        //-------------------------- File server --------------------------------
        //-----------------------------------------------------------------------
        const char* base_path = "/data";
        ESP_ERROR_CHECK(mountHTMLStorage(base_path));

        /* Start the file server */
        ESP_ERROR_CHECK(startHTTPServer(base_path, &wifi));
        ESP_LOGI(TAG, "File server started");

        // for ever while loop to wait for new wifi settings, after successful connection, user must restart the device 
        while (1) {
            if (xQueueReceive(wifiSettingsQueue, &settings, 0) == pdTRUE) {
                wifiState = wifi.connectToWifi(settings.ssid, settings.password);

                if (wifiState == DISCONNECTED) {
                    xEventGroupSetBits(settings.connectionFlag, WIFI_FINISHED);
                }
                else if (wifiState == CONNECTED) {
                    xEventGroupSetBits(settings.connectionFlag, WIFI_CONNECTED_BIT | WIFI_FINISHED);

                    // save the new settings
                    savedSettings.putString(WIFI_SETTINGS_SSID_KEY, settings.ssid);
                    savedSettings.putString(WIFI_SETTINGS_PASSWORD_KEY, settings.password);
                }
                vTaskDelay(50);
            }
        }
    }

    ledParams.pin = greenLed;
    xTaskCreate(
        ledBlinkTask,       /* Function that implements the task. */
        "MQTT_LED",          /* Text name for the task. */
        4096,      /* Stack size in words, not bytes. */
        (void*)&ledParams,    /* Parameter passed into the task. */
        tskIDLE_PRIORITY,/* Priority at which the task is created. */
        &ledTask);      /* Used to pass out the created task's handle. */


    bool connected = mqtt_app_start();
    if (connected) {
        while (!ledTaskFinished) {/*Wait for ledTask to finish, ledTaskFinished is toggled in the led task*/ }
        vTaskDelete(ledTask);
        ledTask = NULL;
        digitalWrite(greenLed, HIGH);
    }

    while (1) {
        mqttFlags = xEventGroupWaitBits(mqttEventGroup, MQTT_DISCONNECTED | MQTT_CONNECTED, pdTRUE, pdFALSE, 0);
        if (mqttFlags == MQTT_DISCONNECTED) {
            ESP_LOGI(TAG, "MQTT disconnected, starting ledTask");
            digitalWrite(greenLed, LOW);
            xTaskCreate(
                ledBlinkTask,       /* Function that implements the task. */
                "MQTT_LED",          /* Text name for the task. */
                4096,      /* Stack size in words, not bytes. */
                (void*)&ledParams,    /* Parameter passed into the task. */
                tskIDLE_PRIORITY,/* Priority at which the task is created. */
                &ledTask);      /* Used to pass out the created task's handle. */
        }
        else if (mqttFlags == MQTT_CONNECTED) {
            ESP_LOGI(TAG, "MQTT connected, deleting ledTask and turning on green led");
            if (ledTask != NULL) {
                ledTaskFinished = true;
                vTaskDelete(ledTask);
                ledTask = NULL;
            }
            digitalWrite(greenLed, HIGH);
        }
        vTaskDelay(1000);
    }

#if 0   // This is for the future, now testing the MQTT before implementing this to the MQTT
    while (1) {
        wifiState = wifi.getWifiState(&settings);
        if (wifiState == DISCONNECTED) {
            ESP_LOGI(TAG, "Wifi in trouble! restarting...");
            esp_restart();
        }
        else if (wifiState == CONNECTING) {
            ESP_LOGI(TAG, "Yrittää yhdistää wifiin...");
        }



        vTaskDelay(1000);
    }
#endif


}


//=======================================================================
//============================ Functions ================================
//=======================================================================
boolean waitForDebugTask() {
    xEventGroupWaitBits(
        taskInitializedGroup, /* The event group being tested. */
        DEBUG_TASK,           /* The bits within the event group to wait for. */
        pdTRUE, /* DEBUG_TASK -bit should be cleared before returning. */
        pdTRUE, /* Don't wait for both bits, either bit will do. */
        portMAX_DELAY); /* Wait a maximum of 100ms for either bit to be set.
                         */
    debug("internetTask started");

    return true;
}

void createAP(Wifi* wifi) {
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(YELLOW_LED_PIN, HIGH);
    wifi->mode(WIFI_MODE_APSTA);
}

void stopAP(Wifi* wifi) {
    digitalWrite(YELLOW_LED_PIN, LOW);
    wifi->mode(WIFI_MODE_STA);
}
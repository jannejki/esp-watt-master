#include "tasks/internetTask.h"
#define GREEN_LED_PIN 4
#define YELLOW_LED_PIN 5
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
    TaskHandle_t ledTask = NULL;
    savedSettings.begin("wifi", false);

    uint8_t greenLed = GREEN_LED_PIN;
    uint8_t yellowLed = YELLOW_LED_PIN;

    pinMode(greenLed, OUTPUT);
    pinMode(yellowLed, OUTPUT);

    String ssid = savedSettings.getString(WIFI_SETTINGS_SSID_KEY);
    String password = savedSettings.getString(WIFI_SETTINGS_PASSWORD_KEY);

    Wifi wifi;

    ledTaskFinished = false;
    ledParams.pin = yellowLed;
    ledParams.delay = 1000;
    ledParams.loopFinished = &ledTaskFinished;

    WifiSettings settings;

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

    //-----------------------------------------------------------------------
   //-------------------------- File server --------------------------------
   //-----------------------------------------------------------------------
    const char* base_path = "/data";
    ESP_ERROR_CHECK(mountHTMLStorage(base_path));

    /* Start the file server */
    ESP_ERROR_CHECK(startHTTPServer(base_path, &wifi));
    ESP_LOGI(TAG, "File server started");

    // If couldn't connect to wifi create AP and wait for settings
    if (wifiState == DISCONNECTED) {
        createAP(&wifi);

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
        "WIFI_LED",          /* Text name for the task. */
        4096,      /* Stack size in words, not bytes. */
        (void*)&ledParams,    /* Parameter passed into the task. */
        tskIDLE_PRIORITY,/* Priority at which the task is created. */
        &ledTask);      /* Used to pass out the created task's handle. */

    while (1) {

        // If new wifi settings are received, try to connect to the new wifi
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

        

        vTaskDelay(1000);
    }


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
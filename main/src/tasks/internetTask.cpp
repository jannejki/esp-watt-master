#include "tasks/internetTask.h"

boolean waitForDebugTask();


//=======================================================================
//======================== WiFI station =================================
//=======================================================================
Preferences wifiSettings;
boolean connectToNewWifi(WifiSettings settings) {
    EventBits_t wifiStateBits = connectToWifi(settings.ssid, settings.password);
    xEventGroupSetBits(settings.connectionFlag, wifiStateBits);

    boolean connected = wifiStateBits & WIFI_CONNECTED_BIT;

    if (connected) {
        wifiSettings.putString(WIFI_SETTINGS_SSID_KEY, settings.ssid);
        wifiSettings.putString(WIFI_SETTINGS_PASSWORD_KEY, settings.password);
        ESP_LOGI(TAG, "Saved new wifi settings! ssid: %s, password: %s", settings.ssid.c_str(), settings.password.c_str());
    }

    return connected;
}

void internetTask(void* params) {
    waitForDebugTask();

    esp_err_t initialized = initWifi();

    if (initialized != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize wifi!");
        ESP_LOGE(TAG, "Restarting in 10");
        for (int i = 10; i > 0; i--) {
            ESP_LOGE(TAG, "%d", i);
        }
    }
    //=======================================================================
    //======================== HTTPD Server =================================
    //=======================================================================
    const char* base_path = "/data";
    ESP_ERROR_CHECK(mountHTMLStorage(base_path));

    /* Start the file server */
    ESP_ERROR_CHECK(startHTTPServer(base_path));
    ESP_LOGI(TAG, "File server started");

    //=======================================================================
    //======================== WiFI station =================================
    //=======================================================================
    wifiSettings.begin("wifi", false);

    String ssid = wifiSettings.getString(WIFI_SETTINGS_SSID_KEY);
    String password = wifiSettings.getString(WIFI_SETTINGS_PASSWORD_KEY);

    WifiSettings newWifiSettings;
    EventBits_t wifiStateBits;
    boolean connected = false;

    while (!connected) {
        if (ssid == "" || password == "") {
            if (xQueueReceive(wifiSettingsQueue, &newWifiSettings, WIFI_TIMEOUT + 1000 / portTICK_PERIOD_MS) == pdTRUE) {
                connected = connectToNewWifi(newWifiSettings);
            }
        }

        else {
            wifiStateBits = connectToWifi(ssid, password);
            connected = wifiStateBits & WIFI_CONNECTED_BIT;
        }
    }

    ESP_LOGI(TAG, "Connected to wifi!", );

    mqtt_app_start();

    while (1) {
        if (xQueueReceive(wifiSettingsQueue, &newWifiSettings, 0) == pdTRUE) {
            ESP_LOGI(TAG, "Received new wifi settings from queue");
            connected = connectToNewWifi(newWifiSettings);
        }

        vTaskDelay(50 / portTICK_PERIOD_MS);
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
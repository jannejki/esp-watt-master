#include "tasks/internetTask.h"
//=======================================================================
//===================== Function definitions ============================
//=======================================================================
boolean waitForDebugTask();
void createAP(Wifi* wifi);
void stopAP(Wifi* wifi);
void apButtonInterrupt(void* arg);

//=======================================================================
//======================== Global variables =============================
//=======================================================================
WifiState wifiState;
Preferences savedSettings;

//=======================================================================
//======================== Internet task ================================
//=======================================================================
void internetTask(void* params) {
    waitForDebugTask();

    gpio_install_isr_service(0); // Select the ESP32 CPU core (0 or 1)
    gpio_config_t ap_button_conf = {
        .pin_bit_mask = 1ULL << CONFIG_WLAN_MODE_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&ap_button_conf);

    boolean apButtonPressed = gpio_get_level((gpio_num_t)CONFIG_WLAN_MODE_PIN);
    savedSettings.begin("wifi", false);

    String ssid = savedSettings.getString(WIFI_SETTINGS_SSID_KEY);
    String password = savedSettings.getString(WIFI_SETTINGS_PASSWORD_KEY);

    Wifi wifi;
    wifiState = DISCONNECTED;

    WifiSettings settings;
    EventBits_t mqttFlags;

    DisplayMessage displayMessage;
    displayMessage.updateType = INTERNET_UPDATE;

    mqttMessage mqttTransmitMessage;

    // If the apButton is not pressed on restart, try to connect to the wifi. Otherwise the immediately create the AP.
    if (!apButtonPressed) {
        wifi.mode(WIFI_MODE_STA);
        wifiState = wifi.connectToWifi(ssid, password);
    } else {
        ESP_LOGI(TAG, "Manually starting WiFi Access point!");
    }


    //-----------------------------------------------------------------------
    //-------------------------- File server --------------------------------
    //-----------------------------------------------------------------------
    const char* base_path = "/data";
    ESP_ERROR_CHECK(mountHTMLStorage(base_path));
    ESP_ERROR_CHECK(startHTTPServer(base_path, &wifi));

    if (wifiState == DISCONNECTED) {
        ESP_LOGI("internetTask", "Creating AP");
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


    bool connected = mqtt_app_start();

#if CONFIG_ENABLE_SSL_CERT_DOWNLOADER

    ESP_LOGI(TAG, "Starting downloading");
    String url = CONFIG_SSL_CERT_DOWNLOAD_URL;
    if (url.length() > 0) {


        String path = "/data/mqtt_cert.crt";
        wifi.downloadFile(url, path);
        ESP_LOGI(TAG, "Downloading finished");
    }
    else {
        ESP_LOGE(TAG, "No URL provided for downloading SSL certificate. Please provide a URL in the menuconfig");
    }
#endif
    while (1) {
        // Check if mqtt is connected
        mqttFlags = xEventGroupWaitBits(mqttEventGroup, MQTT_DISCONNECTED | MQTT_CONNECTED, pdTRUE, pdFALSE, 0);
        // Check if wifi is in trouble
        wifiState = wifi.getWifiState(&settings);
        if (wifiState == DISCONNECTED) {
            ESP_LOGE(TAG, "Wifi in trouble! restarting ESP32...");
            esp_restart();
        }
        else if (wifiState == CONNECTING) {
            ESP_LOGW(TAG, "Wifi lost, trying to connect to wifi...");
        }

        if (xQueueReceive(mqttTransmitQueue, &mqttTransmitMessage, 0) == pdTRUE) {
            mqttSendData(&mqttTransmitMessage);
        }

        vTaskDelay(10);
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
    wifi->mode(WIFI_MODE_APSTA);
}

void stopAP(Wifi* wifi) {
    wifi->mode(WIFI_MODE_STA);
}

void apButtonInterrupt(void* arg) {
    esp_restart();
}
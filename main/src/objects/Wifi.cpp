#include "objects/Wifi.h"

static int s_retry_num = 0;

/* FreeRTOS event group to signal when we are connected*/
EventGroupHandle_t s_wifi_event_group;

void sendWifiStatusToDisplay(boolean wifiConnected) {
    DisplayMessage displayMessage;
    displayMessage.updateType = INTERNET_UPDATE;
    displayMessage.internetConnection = wifiConnected;

    xQueueSend(displayQueue, &displayMessage, 0);
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data) {
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event =
            (wifi_event_ap_staconnected_t*)event_data;
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event =
            (wifi_event_ap_stadisconnected_t*)event_data;
    }
}


esp_err_t _http_event_handler(esp_http_client_event_t* evt)
{
    esp_http_client_event_id_t event = evt->event_id;
    char data[evt->data_len + 1];
    static int total_length = 0;
    switch (event) {
    case HTTP_EVENT_ERROR:
        ESP_LOGI("HTTP_EVENT_HANDLER", "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI("HTTP_EVENT_HANDLER", "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGI("HTTP_EVENT_HANDLER", "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        if (strcasecmp(evt->header_key, "Content-Length") == 0) {
            total_length = atoi(evt->header_value);
            printf("Total content length: %d\n", total_length);
        }
        break;
    case HTTP_EVENT_ON_DATA:
        // Ensure that the received data is null-terminated to treat it as a string
        memcpy(data, evt->data, evt->data_len);
        data[evt->data_len] = '\0';
        // Print only the valid portion of the received data based on the length provided
        printf("Data received: %s\n", data);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI("HTTP_EVENT_HANDLER", "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI("HTTP_EVENT_HANDLER", "HTTP_EVENT_DISCONNECTED");
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGI("HTTP_EVENT_HANDLER", "HTTP_EVENT_REDIRECT");
        break;
    }

    return ESP_OK;
}


static void wifiStationEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_DISCONNECTED) {
        sendWifiStatusToDisplay(false);
        wifi_event_sta_disconnected_t* eventData = (wifi_event_sta_disconnected_t*)event_data;
        ESP_LOGW(TAG_NAME, "Disconnected from AP. Reason: %d", eventData->reason);
        if (eventData->reason == 15) xEventGroupSetBits(s_wifi_event_group, WIFI_WRONG_PASSWORD_BIT);

        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGW(TAG_NAME,
                "Could not connect to access point. Retry %d/%d...",
                s_retry_num, EXAMPLE_ESP_MAXIMUM_RETRY);
        }
        else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FINISHED);
            ESP_LOGW(TAG_NAME, "connect to the AP fail");
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        s_retry_num = 0;
        xEventGroupClearBits(s_wifi_event_group, WIFI_WRONG_PASSWORD_BIT);
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FINISHED);
        sendWifiStatusToDisplay(true);
    }
}

Wifi::Wifi() {
    // TODO Auto-generated constructor stub
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
}

Wifi::~Wifi() {
    // TODO Auto-generated destructor stub
}


void Wifi::mode(wifi_mode_t mode) {
    if (wifiState != NOT_INITIALIZED) {
        esp_wifi_deinit();
        wifiState = NOT_INITIALIZED;
    }
    else {
        esp_wifi_disconnect();
    }

    if (mode == WIFI_MODE_STA || mode == WIFI_MODE_APSTA) {
        if (sta_netif == NULL) {
            sta_netif = esp_netif_create_default_wifi_sta();
        }
        esp_wifi_set_mode(mode);
    }

    if (mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA) {
        if (ap_netif == NULL) {
            ap_netif = esp_netif_create_default_wifi_ap();
            wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
            ESP_ERROR_CHECK(esp_wifi_init(&cfg));
            // Initialize the default event loop

            ESP_ERROR_CHECK(esp_event_handler_instance_register(
                WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

            wifi_config_t wifi_config = {
                .ap =
                    {
                        .ssid = EXAMPLE_ESP_WIFI_SSID,
                        .password = EXAMPLE_ESP_WIFI_PASS,
                        .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
                        .channel = EXAMPLE_ESP_WIFI_CHANNEL,
                        .authmode = WIFI_AUTH_WPA2_PSK,
                        .max_connection = EXAMPLE_MAX_STA_CONN,
                        .pmf_cfg =
                            {
                                .required = true,
                            },
                    },
            };
            if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
                wifi_config.ap.authmode = WIFI_AUTH_OPEN;
            }

            /* DHCP settings */
            ESP_ERROR_CHECK(esp_netif_dhcps_stop(ap_netif));

            esp_netif_ip_info_t ip_info;

            IP4_ADDR(&ip_info.ip, 192, 168, 101, 1);
            IP4_ADDR(&ip_info.gw, 192, 168, 101, 1);
            IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);

            ESP_ERROR_CHECK(esp_netif_set_ip_info(ap_netif, &ip_info));
            esp_netif_dhcps_start(ap_netif);
            /*  DHCP settings end */

            esp_wifi_set_mode(mode);
            ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

            ESP_LOGI(TAG_NAME, "Wifi access point started! SSID: %s password: %s\n\r", EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
        }
        else {
            esp_wifi_disconnect();
        }
    }


    ESP_ERROR_CHECK(esp_wifi_start());
    wifiState = DISCONNECTED;
}

esp_err_t Wifi::changeStationSettings(String& ssid, String& password) {
    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.sta.ssid, ssid.c_str(),
        sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, password.c_str(),
        sizeof(wifi_config.sta.password));

    wifi_config.sta.threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD;
    wifi_config.sta.sae_pwe_h2e = ESP_WIFI_SAE_MODE;

    strncpy((char*)wifi_config.sta.sae_h2e_identifier, EXAMPLE_H2E_IDENTIFIER,
        sizeof(wifi_config.sta.sae_h2e_identifier));

    // Setting configs, then starting to connect
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    return ESP_OK;
}

/**
 * @brief Connect to the WiFi network
 * @returns WifiState -  CONNECTED = 1, CONNECTING = 0, DISCONNECTED = -1, NOT_INITIALIZED = -2
*/
WifiState Wifi::connectToWifi(String& ssid, String& password) {
    s_retry_num = 0;
    // If event group is null, then this is the first time trying to connect to Access Point
    if (s_wifi_event_group == NULL) {
        // If the event group hasn't been created yet, create it
        s_wifi_event_group = xEventGroupCreate();

        esp_event_handler_instance_t instance_any_id;

        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            WIFI_EVENT, ESP_EVENT_ANY_ID, &wifiStationEventHandler, NULL,
            &instance_any_id));

        esp_event_handler_instance_t instance_got_ip;
        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            IP_EVENT, IP_EVENT_STA_GOT_IP, &wifiStationEventHandler, NULL,
            &instance_got_ip));
    }
    else {
        // Clear the bits from the previous connection
        xEventGroupClearBits(s_wifi_event_group, 0b1111);
    }

    // If Wifi is connected, then it must be disconnected before connecting to new one
    if (wifiState == CONNECTED) {
        // Disconnect from the current WiFi network
        esp_wifi_disconnect();
        wifiState = DISCONNECTED;
    }

    ESP_ERROR_CHECK(changeStationSettings(ssid, password));

    // This was ESP_ERROR_CHECK before, but it caused random crashes on startup, then after couple restarts, it just works fine.
    // I inserted the if else statements to check the return value of the function and but now this works fine all the time apparently.
    esp_err_t connecting = esp_wifi_connect();

    if (connecting == ESP_OK) {
        ESP_LOGI(TAG_NAME, "Connecting to SSID: %s password: %s\n\r", ssid.c_str(), password.c_str());
    }
    else if (connecting == ESP_ERR_WIFI_NOT_INIT) {
        ESP_LOGW(TAG_NAME, "Wifi not initialized");
        return NOT_INITIALIZED;
    }
    else if (connecting == ESP_ERR_WIFI_NOT_STARTED) {
        ESP_LOGW(TAG_NAME, "Wifi not started");
        return NOT_INITIALIZED;
    }
    else if (connecting == ESP_ERR_WIFI_CONN) {
        ESP_LOGW(TAG_NAME, "Wifi connection error");
    }
    else if (connecting == ESP_ERR_WIFI_SSID) {
        ESP_LOGW(TAG_NAME, "Wifi SSID not found");
        return DISCONNECTED;
    }
    else {
        ESP_LOGW(TAG_NAME, "Unknown Wifi error");
        return NOT_INITIALIZED;
    }

    // Waiting for the wifiStationEventHandler to set the WIFI_CONNECTED_BIT or WIFI_FAIL_BIT
    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group, WIFI_FINISHED, pdFALSE,
        pdFALSE, WIFI_TIMEOUT / portTICK_PERIOD_MS);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG_NAME, "Connected to SSID: %s\n\r", ssid.c_str());
        wifiState = CONNECTED;
    }
    else if (bits & WIFI_WRONG_PASSWORD_BIT) {
        ESP_LOGE(TAG_NAME, "Wrong password for SSID: %s\n\r", ssid.c_str());
        wifiState = DISCONNECTED;
    }
    else {
        ESP_LOGE(TAG_NAME, "Failed to connect to SSID: %s password: %s\n\r", ssid.c_str(), password.c_str());
        wifiState = DISCONNECTED;
    }

    return wifiState;
}


WifiState Wifi::getWifiState(WifiSettings* settings) {
    wifi_config_t wifi_config;
    wifi_ap_record_t ap_info;
    esp_err_t ret;

    if (wifiState == NOT_INITIALIZED) return NOT_INITIALIZED;

    esp_err_t success = esp_wifi_get_config(WIFI_IF_STA, &wifi_config);
    settings->ssid = (char*)wifi_config.sta.ssid;

    ret = esp_wifi_sta_get_ap_info(&ap_info);

    // if ret == ESP_OK or the ESP is still trying to connect to the AP, then nothing to worry. 
    // If it has tried to connected maximum times and failed, then return DISCONNECTED

    if (ret == ESP_OK) {
        // ESP32 is connected to an AP
        wifiState = CONNECTED;
    }

    else if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
        // ESP32 is trying to connect to an AP
        wifiState = CONNECTING;
    }

    else {
        // ESP32 is not connected to an AP
        wifiState = DISCONNECTED;
    }

    return wifiState;

}

esp_err_t Wifi::getIPInfo(esp_netif_ip_info_t* ip_info) {
    esp_netif_t* sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (sta_netif) {
        return esp_netif_get_ip_info(sta_netif, ip_info);
    }
    return ESP_FAIL;
}


esp_err_t Wifi::downloadFile(String url, String data) {
    esp_http_client_config_t config = {
        .url = url.c_str(),
        .event_handler = _http_event_handler,
        .user_data = NULL,  // Not needed for POST request
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    // Set the HTTP method to POST
    esp_http_client_set_method(client, HTTP_METHOD_POST);

    // Set the request body data
    esp_http_client_set_post_field(client, data.c_str(), data.length());

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG_NAME, "Data uploaded successfully");
    }
    else {
        ESP_LOGE(TAG_NAME, "Failed to upload data");
    }

    esp_http_client_cleanup(client);
    return err;
}

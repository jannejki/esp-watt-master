#include "objects/Wifi.h"

static int s_retry_num = 0;

/* FreeRTOS event group to signal when we are connected*/
EventGroupHandle_t s_wifi_event_group;


static void wifi_event_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data) {
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event =
            (wifi_event_ap_staconnected_t*)event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac),
            event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event =
            (wifi_event_ap_stadisconnected_t*)event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d", MAC2STR(event->mac),
            event->aid);
    }
}

static void wifiStationEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_DISCONNECTED) {

        wifi_event_sta_disconnected_t* eventData = (wifi_event_sta_disconnected_t*)event_data;
        ESP_LOGI(TAG, "Disconnected from AP. Reason: %d", eventData->reason);
        if (eventData->reason == 15) xEventGroupSetBits(s_wifi_event_group, WIFI_WRONG_PASSWORD_BIT);

        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG,
                "Could not connect to access point. Retry %d/%d...\n\r",
                s_retry_num, EXAMPLE_ESP_MAXIMUM_RETRY);
        }
        else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FINISHED);

            ESP_LOGI(TAG, "connect to the AP fail");
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        s_retry_num = 0;
        ESP_LOGI(TAG, "got ip");
        xEventGroupClearBits(s_wifi_event_group, WIFI_WRONG_PASSWORD_BIT);
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FINISHED);
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
    ESP_LOGI(TAG, "Wifi initialized");
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

            /* STATIC IP BEGIN*/
            ESP_ERROR_CHECK(esp_netif_dhcps_stop(ap_netif));

            esp_netif_ip_info_t ip_info;

            IP4_ADDR(&ip_info.ip, 192, 168, 101, 1);
            IP4_ADDR(&ip_info.gw, 192, 168, 101, 1);
            IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);

            ESP_ERROR_CHECK(esp_netif_set_ip_info(ap_netif, &ip_info));
            esp_netif_dhcps_start(ap_netif);
            /* STATIC IP ENDS*/
            esp_wifi_set_mode(mode);
            ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

            ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
                EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS,
                EXAMPLE_ESP_WIFI_CHANNEL);
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
        ESP_LOGI(TAG, "Disconnecting from current network");
        esp_wifi_disconnect();
        wifiState = DISCONNECTED;
    }

    ESP_ERROR_CHECK(changeStationSettings(ssid, password));
    ESP_ERROR_CHECK(esp_wifi_connect());

    // Waiting for the wifiStationEventHandler to set the WIFI_CONNECTED_BIT or WIFI_FAIL_BIT
    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group, WIFI_FINISHED, pdFALSE,
        pdFALSE, WIFI_TIMEOUT / portTICK_PERIOD_MS);

    ESP_LOGI(TAG, "Bits: %d", (uint8_t)bits);
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to SSID: %s\n\r", ssid.c_str());
        wifiState = CONNECTED;
    }
    else {
        ESP_LOGI(TAG, "Failed to connect to SSID: %s password: %s\n\r", ssid.c_str(), password.c_str());
        wifiState = DISCONNECTED;
    }

    return wifiState;
}


WifiState Wifi::getWifiState(WifiSettings* settings) {
    wifi_config_t wifi_config;
    wifi_ap_record_t ap_info;
    esp_err_t ret;

   // ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &wifi_config));
   // settings->ssid = (char*)wifi_config.sta.ssid;

    //  ret = esp_wifi_sta_get_ap_info(&ap_info);
    return wifiState;
#if 0
    if (ret == ESP_OK) {
        // ESP32 is connected to an AP
        wifiState = CONNECTED;
    }
    else {
        // ESP32 is not connected to an AP
        wifiState = DISCONNECTED;
    }

    return wifiState;
#endif
}

esp_err_t Wifi::getIPInfo(esp_netif_ip_info_t* ip_info) {
    esp_netif_t* sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (sta_netif) {
        return esp_netif_get_ip_info(sta_netif, ip_info);
    }
    return ESP_FAIL;
}

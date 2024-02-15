#if 0
#include "utils/wifi.h"
#define TAG "wifi"

#define EXAMPLE_ESP_WIFI_SSID CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_WIFI_CHANNEL CONFIG_ESP_WIFI_CHANNEL
#define EXAMPLE_MAX_STA_CONN CONFIG_ESP_MAX_STA_CONN

#ifndef ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD
#endif

#ifndef ESP_WIFI_SAE_MODE
#define ESP_WIFI_SAE_MODE
#endif

#ifndef CONFIG_ESP_MAXIMUM_RETRY
#define CONFIG_ESP_MAXIMUM_RETRY 7
#endif

#define EXAMPLE_ESP_MAXIMUM_RETRY CONFIG_ESP_MAXIMUM_RETRY

#if CONFIG_ESP_WPA3_SAE_PWE_HUNT_AND_PECK
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
#define EXAMPLE_H2E_IDENTIFIER ""
#elif CONFIG_ESP_WPA3_SAE_PWE_HASH_TO_ELEMENT
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HASH_TO_ELEMENT
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#elif CONFIG_ESP_WPA3_SAE_PWE_BOTH
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#endif
#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

static int s_retry_num = 0;

/* FreeRTOS event group to signal when we are connected*/
EventGroupHandle_t s_wifi_event_group;

WifiState wifiState = NOT_INITIALIZED;

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

static void wifiStationEventHandler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data) {
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

esp_err_t getIPInfo(esp_netif_ip_info_t* ip_info) {
    esp_netif_t* sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (sta_netif) {
        return esp_netif_get_ip_info(sta_netif, ip_info);
    }
    return ESP_FAIL;
}

WifiState getWifiState(WifiSettings* settings) {
    wifi_config_t wifi_config;
    ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &wifi_config));
    settings->ssid = (char*)wifi_config.sta.ssid;
    return wifiState;
}

esp_err_t initWifi() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_t* sta_netif = esp_netif_create_default_wifi_sta();

#if 0   // This is not needed, as the default mode is WIFI_MODE_STA
    esp_netif_t* ap_netif =
        esp_netif_create_default_wifi_ap();  // Initialize ap_netif properly
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

    // ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

    esp_wifi_set_mode(WIFI_MODE_APSTA);

    /* STATIC IP BEGIN*/
    ESP_ERROR_CHECK(esp_netif_dhcps_stop(ap_netif));

    esp_netif_ip_info_t ip_info;

    IP4_ADDR(&ip_info.ip, 192, 168, 1, 1);
    IP4_ADDR(&ip_info.gw, 192, 168, 1, 1);
    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);

    ESP_ERROR_CHECK(esp_netif_set_ip_info(ap_netif, &ip_info));
    esp_netif_dhcps_start(ap_netif);
    /* STATIC IP ENDS*/

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
        EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS,
        EXAMPLE_ESP_WIFI_CHANNEL);
#else
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    esp_wifi_set_mode(WIFI_MODE_STA);
#endif
    ESP_ERROR_CHECK(esp_wifi_start());
    return ESP_OK;
}

void initWifiAP() {

}

EventBits_t connectToWifi(const String& ssid, const String& password) {
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

    return bits;
}

#endif
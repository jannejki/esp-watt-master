#include "objects/Wifi.h"

//======================================================//
//================  Global functions  ==================//
//======================================================//

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

static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT &&
               event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

//======================================================//
//===================== WiFi class =====================//
//======================================================//
Wifi::Wifi(uint8_t* _ssid, uint8_t* _password)
    : ssid(_ssid), password(_password) {}

Wifi::~Wifi() {}

void Wifi::init(void) {
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.sta.ssid, (char*)ssid,
            sizeof(wifi_config.sta.ssid));

    strncpy((char*)wifi_config.sta.password, (char*)password,
            sizeof(wifi_config.sta.password));

    wifi_config.sta.threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD;
    wifi_config.sta.sae_pwe_h2e = ESP_WIFI_SAE_MODE;

    strncpy((char*)wifi_config.sta.sae_h2e_identifier, EXAMPLE_H2E_IDENTIFIER,
            sizeof(wifi_config.sta.sae_h2e_identifier));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT)
     * or connection failed for the maximum number of re-tries (WIFI_FAIL_BIT).
     * The bits are set by event_handler() (see above) */
    EventBits_t bits;

    while (1) {
        bits = xEventGroupWaitBits(s_wifi_event_group,
                                   WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE,
                                   pdFALSE, 1000 / portTICK_PERIOD_MS);
        if (bits & WIFI_CONNECTED_BIT || bits & WIFI_FAIL_BIT) {
            break;
        } else {
            debug("Waiting for wifi connection...\n\r");
        }
    }
    /* xEventGroupWaitBits() returns the bits before the call returned, hence we
     * can test which event actually happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        char msg[64];
        sprintf(msg, "Connected to SSID: %s\n", ssid);
        debug(msg);
        esp_netif_ip_info_t ip_info;
        esp_netif_t* sta_netif =
            esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
        if (sta_netif) {
            esp_netif_get_ip_info(sta_netif, &ip_info);

            sprintf(msg, "Got IP: %s\n",
                    ip4addr_ntoa((ip4_addr_t*)&ip_info.ip));

            debug(msg);
        }
    } else if (bits & WIFI_FAIL_BIT) {
        debug("Failed to connect to SSID: %s password: %s\n\r", ssid, password);
    } else {
        debug("UNEXPECTED EVENT\n\r");
    }
}

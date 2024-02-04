#include "tasks/internetTask.h"

boolean waitForDebugTask();
boolean connectToWifi(Wifi* wifi, String ssid, String password);
boolean startWifiAccessPoint(Wifi* wifi);
#define EXAMPLE_ESP_WIFI_SSID CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_WIFI_CHANNEL CONFIG_ESP_WIFI_CHANNEL
#define EXAMPLE_MAX_STA_CONN CONFIG_ESP_MAX_STA_CONN

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event =
            (wifi_event_ap_staconnected_t*)event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac),
                 event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event =
            (wifi_event_ap_stadisconnected_t*)event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d", MAC2STR(event->mac),
                 event->aid);
    }
}

void internetTask(void* params) {
    waitForDebugTask();
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

    ESP_ERROR_CHECK(esp_wifi_start());

    //=======================================================================
    //======================== HTTPD Server =================================
    //=======================================================================
    const char* base_path = "/data";
    ESP_ERROR_CHECK(example_mount_storage(base_path));

    /* Start the file server */
    ESP_ERROR_CHECK(example_start_file_server(base_path));
    ESP_LOGI(TAG, "File server started");

    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

boolean waitForDebugTask() {
    xEventGroupWaitBits(
        taskInitializedGroup, /* The event group being tested. */
        DEBUG_TASK,           /* The bits within the event group to wait for. */
        pdTRUE, /* DEBUG_TASK -bit should be cleared before returning. */
        pdTRUE, /* Don't wait for both bits, either bit will do. */
        portMAX_DELAY); /* Wait a maximum of 100ms for either bit to be set. */
    debug("internetTask started\n\r");

    return true;
}

boolean connectToWifi(Wifi* wifi, String ssid, String password) {
    boolean connected =
        wifi->initAsStation((uint8_t*)ssid.c_str(), (uint8_t*)password.c_str());

    if (!connected) {
        debug("Couldn't connect to wifi!\n\r");
        return false;
    }

    esp_netif_ip_info_t ip_info;
    if (wifi->getIPInfo(&ip_info) == ESP_OK) {
        char* ip = ip4addr_ntoa((ip4_addr_t*)&ip_info.ip);
        debug("IP Address: %s\n\r", ip);
        return true;
    } else {
        debug("Failed to get IP info\n\r");
        return false;
    }
}

boolean startWifiAccessPoint(Wifi* wifi) {
    debug("Starting wifi access point!(ei vielä käytössä)\n\r");
    wifi->initAsAccessPoint();
    return true;
}
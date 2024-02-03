#include "tasks/internetTask.h"

boolean waitForDebugTask();
boolean connectToWifi(Wifi* wifi, String ssid, String password);
boolean startWifiAccessPoint(Wifi *wifi);

void internetTask(void* params) {
    Preferences wifiSetttings;
    wifiSetttings.begin("wifi", false);

    waitForDebugTask();
    Wifi wifi;

    String ssid = wifiSetttings.getString(WIFI_SETTINGS_SSID_KEY);
    String password = wifiSetttings.getString(WIFI_SETTINGS_PASSWORD_KEY);

    debug("ssid: %s \n\r", ssid);
    debug("password: %s\n\r", password);

    if (ssid == "" || password == "") {
        debug("ssid or password is empty! starting wifi access point!\n\r");
        startWifiAccessPoint(&wifi);
    } else {
        boolean connected = connectToWifi(&wifi, ssid, password);
    }

    while (1) {
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

boolean startWifiAccessPoint(Wifi *wifi) {
    debug("Starting wifi access point!(ei vielä käytössä)\n\r");
    wifi->initAsAccessPoint();
    return true;
}
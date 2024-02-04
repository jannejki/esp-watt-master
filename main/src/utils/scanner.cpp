#include "utils/scanner.h"


static const char* TAG = "Scanner";
/**
 * @param found_ssids: array of found ssids
 */
int wifi_scan(wifi_ap_record_t* ap_info) {
    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    //wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    uint16_t ap_count = 0;

    wifi_scan_config_t scan_config;
    scan_config.ssid = 0;
    scan_config.bssid = 0;
    scan_config.channel = 0;  // 0 means scan all channels
    scan_config.show_hidden = true;
    scan_config.scan_type = WIFI_SCAN_TYPE_ACTIVE;
    scan_config.scan_time.active.min =
        120;  // minimum time in milliseconds to spend on each channel
    scan_config.scan_time.active.max =
        300;  // maximum time in milliseconds to spend on each channel

    esp_wifi_scan_start(&scan_config, true);

    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ESP_LOGI(TAG, "Total APs scanned = %u", ap_count);


    // stop wifi scan
    esp_err_t result = esp_wifi_scan_stop();
    if(result != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop wifi scan");
        
    } else {
        ESP_LOGI(TAG, "Wifi scan stopped");
    }

    return ap_count;
}
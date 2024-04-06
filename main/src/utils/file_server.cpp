/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
 /* HTTP File Server Example

    This example code is in the Public Domain (or CC0 licensed, at your option.)

    Unless required by applicable law or agreed to in writing, this
    software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
    CONDITIONS OF ANY KIND, either express or implied.
 */
#include "utils/file_serving_example_common.h"

 /* Max length a file path can have on storage */
#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + CONFIG_SPIFFS_OBJ_NAME_LEN)

/* Max size of an individual file. Make sure this
 * value is same as that set in upload_script.html */
#define MAX_FILE_SIZE (200 * 1024)  // 200 KB
#define MAX_FILE_SIZE_STR "200KB"

 /* Scratch buffer size */
#define SCRATCH_BUFSIZE 8192

struct file_server_data {
    /* Base path of file storage */
    char base_path[ESP_VFS_PATH_MAX + 1];

    /* Scratch buffer for temporary storage during file transfer */
    char scratch[SCRATCH_BUFSIZE];
};
static const char* TAG = "file_server";
Wifi* g_wifi;
/* Handler to redirect incoming GET request for /index.html to /
 * This can be overridden by uploading file with same name */
static esp_err_t index_html_get_handler(httpd_req_t* req) {
    httpd_resp_set_status(req, "307 Temporary Redirect");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);  // Response body can be empty
    return ESP_OK;
}

/* Handler to respond with an icon file embedded in flash.
 * Browsers expect to GET website icon at URI /favicon.ico.
 * This can be overridden by uploading file with same name */
static esp_err_t favicon_get_handler(httpd_req_t* req) {
    extern const unsigned char favicon_ico_start[] asm(
        "_binary_favicon_ico_start");
    extern const unsigned char favicon_ico_end[] asm("_binary_favicon_ico_end");
    const size_t favicon_ico_size = (favicon_ico_end - favicon_ico_start);
    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_send(req, (const char*)favicon_ico_start, favicon_ico_size);
    return ESP_OK;
}

/* Send HTTP response with a run-time generated html consisting of
 * a list of all files and folders under the requested path.
 * In case of SPIFFS this returns empty list when path is any
 * string other than '/', since SPIFFS doesn't support directories */
static esp_err_t http_resp_dir_html(httpd_req_t* req, const char* dirpath) {
    /* Send HTML file header */
    httpd_resp_sendstr_chunk(req, "<!DOCTYPE html><html><body>");

    /* Get handle to embedded file upload script */
    extern const unsigned char upload_script_start[] asm(
        "_binary_upload_script_html_start");
    extern const unsigned char upload_script_end[] asm(
        "_binary_upload_script_html_end");
    const size_t upload_script_size = (upload_script_end - upload_script_start);

    /* Add file upload form and script which on execution sends a POST request
     * to /upload */
    httpd_resp_send_chunk(req, (const char*)upload_script_start,
        upload_script_size);

    WifiSettings settings;
    WifiState state = g_wifi->getWifiState(&settings);

    if (state == CONNECTED) {
        esp_netif_ip_info_t ip_info;
        g_wifi->getIPInfo(&ip_info);
        char* ip = ip4addr_ntoa((ip4_addr_t*)&ip_info.ip);
        char response[200];  // Adjust the size according to your needs
        snprintf(response, sizeof(response),
            "<script>"
            "let span = document.getElementById('connected');"
            "span.innerHTML = 'Yhdistetty verkkoon: %s<br>IP-osoite: %s';"
            "span.style.display = 'block';"
            "</script>",
            settings.ssid.c_str(), ip);
        httpd_resp_sendstr_chunk(req, response);
    }
    else {
        httpd_resp_sendstr_chunk(
            req,
            "<script>"
            "document.getElementById('not_connected').style.display = 'block';"
            "</script>");
    }

    httpd_resp_sendstr_chunk(req, "</html>");
    /* Send empty chunk to signal HTTP response completion */
    ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, NULL));

    return ESP_OK;
}

#define IS_FILE_EXT(filename, ext) \
    (strcasecmp(&filename[strlen(filename) - sizeof(ext) + 1], ext) == 0)

/* Set HTTP response content type according to file extension */
static esp_err_t set_content_type_from_file(httpd_req_t* req,
    const char* filename) {
    if (IS_FILE_EXT(filename, ".pdf")) {
        return httpd_resp_set_type(req, "application/pdf");
    }
    else if (IS_FILE_EXT(filename, ".html")) {
        return httpd_resp_set_type(req, "text/html");
    }
    else if (IS_FILE_EXT(filename, ".jpeg")) {
        return httpd_resp_set_type(req, "image/jpeg");
    }
    else if (IS_FILE_EXT(filename, ".ico")) {
        return httpd_resp_set_type(req, "image/x-icon");
    }
    /* This is a limited set only */
    /* For any other type always set as plain text */
    return httpd_resp_set_type(req, "text/plain");
}

/* Copies the full path into destination buffer and returns
 * pointer to path (skipping the preceding base path) */
static const char* get_path_from_uri(char* dest, const char* base_path,
    const char* uri, size_t destsize) {
    const size_t base_pathlen = strlen(base_path);
    size_t pathlen = strlen(uri);

    const char* quest = strchr(uri, '?');
    if (quest) {
        pathlen = MIN(pathlen, quest - uri);
    }
    const char* hash = strchr(uri, '#');
    if (hash) {
        pathlen = MIN(pathlen, hash - uri);
    }

    if (base_pathlen + pathlen + 1 > destsize) {
        /* Full path string won't fit into destination buffer */
        return NULL;
    }

    /* Construct full path (base + path) */
    strcpy(dest, base_path);
    strlcpy(dest + base_pathlen, uri, pathlen + 1);

    /* Return pointer to path, skipping the base */
    return dest + base_pathlen;
}

/* Handler to download a file kept on the server */
static esp_err_t index_handler(httpd_req_t* req) {
    char filepath[FILE_PATH_MAX];
    FILE* fd = NULL;
    struct stat file_stat;

    const char* filename = get_path_from_uri(
        filepath, ((struct file_server_data*)req->user_ctx)->base_path,
        req->uri, sizeof(filepath));
    if (!filename) {
        ESP_LOGE(TAG, "Filename is too long");
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
            "Filename too long");
        return ESP_FAIL;
    }

    /* If name has trailing '/', respond with directory contents */
    if (filename[strlen(filename) - 1] == '/') {
        ESP_LOGI(TAG, "Sending HTML page");
        return http_resp_dir_html(req, filepath);
    }
    else {
        // return 404
        // LOG the filename to ESP_LOG
        ESP_LOGW(TAG, "File name: \'%s\' not found from filesystem", filename);
        return httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found");
    }
}

/* Handler to download a file kept on the server */
static esp_err_t access_points_handler(httpd_req_t* req) {
    // start wifi scanning for access points

    ESP_LOGI(TAG, "Starting wifi scanning");
    wifi_ap_record_t* found_ssids = (wifi_ap_record_t*)malloc(
        DEFAULT_SCAN_LIST_SIZE * sizeof(wifi_ap_record_t));
    int amount = wifi_scan(found_ssids);
    ESP_LOGI(TAG, "Total APs to be printed = %u", amount);
    int maxAmount = amount > 10 ? DEFAULT_SCAN_LIST_SIZE : amount;

    for (int i = 0; i < maxAmount; i++) {
        if (strcmp((char*)found_ssids[i].ssid, "") != 0) {
            char buff[128];
            sprintf(buff, "%s:%d;", found_ssids[i].ssid, found_ssids[i].rssi);
            ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, buff));
        }
    }

    free(found_ssids);
    /* Send empty chunk to signal HTTP response completion */
    ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, NULL));
    return ESP_OK;
}

/* Handler to download a file kept on the server */
static esp_err_t access_point_post_handler(httpd_req_t* req) {
    int remaining = req->content_len;
    /* Allocate buffer to hold the received data */
    char buf[req->content_len + 1];  // +1 for null terminator
    int received;
    int index = 0;  // Index to keep track of buffer position

    while (remaining > 0) {
        if ((received = httpd_req_recv(req, buf + index,
            MIN(remaining, SCRATCH_BUFSIZE))) <= 0) {
            if (received == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Retry if timeout occurred */
                continue;
            }
        }

        remaining -= received;
        index += received;
    }

    // Null-terminate the received buffer to treat it as a string
    buf[index] = '\0';

    char* ssid = strtok(buf, "=");
    char* pwd = strtok(NULL, "=");

    WifiSettings wifiSettings;
    // ssid and pwd must be in a String format
    wifiSettings.ssid = String(ssid);
    wifiSettings.password = String(pwd);
    EventGroupHandle_t connectionFlag = xEventGroupCreate();
    wifiSettings.connectionFlag = connectionFlag;
    // Clear the bits in the event group
    xQueueSend(wifiSettingsQueue, &wifiSettings, portMAX_DELAY);

    EventBits_t bits =
        xEventGroupWaitBits(connectionFlag, WIFI_FINISHED,
            pdFALSE, pdFALSE, WIFI_TIMEOUT + 1000 / portTICK_PERIOD_MS);

    if (bits & WIFI_CONNECTED_BIT) {
        WifiSettings settings;
        g_wifi->getWifiState(&settings);

        esp_netif_ip_info_t ip_info;
        g_wifi->getIPInfo(&ip_info);
        char* ip = ip4addr_ntoa((ip4_addr_t*)&ip_info.ip);
        char response[200];  // Adjust the size according to your needs
        snprintf(response, sizeof(response),
            "{\"ssid\":\"%s\",\"ipaddress\":\"%s\"}",
            settings.ssid.c_str(), ip);

        httpd_resp_sendstr_chunk(req, response);

    }
    else {
        if (bits & WIFI_WRONG_PASSWORD_BIT) {
            httpd_resp_set_status(req, "401");
            httpd_resp_sendstr_chunk(req, "Wrong password");
        }

        // Something else went wrong
        else {
            httpd_resp_set_status(req, "500");
            httpd_resp_sendstr_chunk(req, "Failed to connect to AP");
        }
    }

    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}

static esp_err_t restart_command_handler(httpd_req_t* req) {
    ESP_LOGI(TAG, "Received command to restart");
    httpd_resp_sendstr_chunk(req, "Restarting");
    httpd_resp_sendstr_chunk(req, NULL);
    vTaskDelay(5000);
    esp_restart();
    return ESP_OK;
}


/* Function to start the file server */
esp_err_t startHTTPServer(const char* base_path, Wifi* wifi) {
    struct file_server_data* server_data = NULL;
    g_wifi = wifi;
    if (server_data) {
        ESP_LOGE(TAG, "File server already started");
        return ESP_ERR_INVALID_STATE;
    }

    server_data =
        (struct file_server_data*)calloc(1, sizeof(struct file_server_data));
    if (!server_data) {
        ESP_LOGE(TAG, "Failed to allocate memory for server data");
        return ESP_ERR_NO_MEM;
    }
    strlcpy(server_data->base_path, base_path, sizeof(server_data->base_path));

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 40960;  // 40 KB stack size

    /* Use the URI wildcard matching function in order to
     * allow the same handler to respond to multiple different
     * target URIs which match the wildcard scheme */
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(TAG, "Starting HTTP Server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start file server!");
        return ESP_FAIL;
    }

    /* URI handler for getting uploaded files */
    httpd_uri_t index = {
        .uri = "/",  // Match all URIs of type /path/to/file
        .method = HTTP_GET,
        .handler = index_handler,
        .user_ctx = server_data  // Pass server data as context
    };

    /* URI handler for getting uploaded files */
    httpd_uri_t access_points = {
        .uri = "/access_points",  // Match all URIs of type /path/to/file
        .method = HTTP_GET,
        .handler = access_points_handler,
        .user_ctx = server_data  // Pass server data as context
    };

    /* URI handler for uploading files to server */
    httpd_uri_t access_points_post = {
        .uri = "/access_points",  // Match all URIs of type /upload/path/to/file
        .method = HTTP_POST,
        .handler = access_point_post_handler,
        .user_ctx = server_data  // Pass server data as context
    };

    /* URI handler for uploading files to server */
    httpd_uri_t restart_command_post = {
        .uri = "/restart",  // Match all URIs of type /upload/path/to/file
        .method = HTTP_POST,
        .handler = restart_command_handler,
        .user_ctx = server_data  // Pass server data as context
    };

    httpd_register_uri_handler(server, &index);
    httpd_register_uri_handler(server, &access_points);
    httpd_register_uri_handler(server, &access_points_post);
    httpd_register_uri_handler(server, &restart_command_post);
    return ESP_OK;
}

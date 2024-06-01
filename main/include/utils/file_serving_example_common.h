/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
 /* HTTP File Server Example, common declarations

    This example code is in the Public Domain (or CC0 licensed, at your option.)

    Unless required by applicable law or agreed to in writing, this
    software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
    CONDITIONS OF ANY KIND, either express or implied.
 */
#ifndef FILE_SERVING_EXAMPLE_COMMON_H
#define FILE_SERVING_EXAMPLE_COMMON_H
#include "../main.h"
#include "FreeRTOS/event_groups.h"
#include "esp_err.h"
#include "sdkconfig.h"

#include "utils/scanner.h"
#include "utils/wifi.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/unistd.h>

#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_vfs.h"

#include "objects/Wifi.h"
esp_err_t mountHTMLStorage(const char* base_path);
esp_err_t startHTTPServer(const char* base_path, Wifi* wifi);
const char* read_certificate_from_file(const char* path);
#endif  // FILE_SERVING_EXAMPLE_COMMON_H
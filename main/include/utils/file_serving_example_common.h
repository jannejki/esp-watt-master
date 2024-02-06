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

#pragma once

#include "../main.h"
#include "FreeRTOS/event_groups.h"
#include "esp_err.h"
#include "sdkconfig.h"
#ifdef __cplusplus
extern "C" {
#endif
   esp_err_t mountHTMLStorage(const char* base_path);
   esp_err_t startHTTPServer(const char* base_path);

#ifdef __cplusplus
}
#endif

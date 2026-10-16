// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "esp_err.h"

#include <cstdint>

struct wifi_config_t {
    struct {
        char ssid[32];
    } sta;
};

typedef int wifi_interface_t;
#define WIFI_IF_STA 0

inline esp_err_t esp_wifi_get_config(wifi_interface_t, wifi_config_t *)
{
    return ESP_OK;
}
inline esp_err_t esp_wifi_disconnect(void)
{
    return ESP_OK;
}
inline esp_err_t esp_wifi_set_config(wifi_interface_t, wifi_config_t *)
{
    return ESP_OK;
}

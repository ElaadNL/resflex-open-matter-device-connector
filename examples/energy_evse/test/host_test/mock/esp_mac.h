// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "esp_err.h"

#include <cstdint>

#define ESP_MAC_WIFI_STA 0

esp_err_t esp_read_mac(uint8_t *mac, int type);

bool &unit_test_esp_read_mac_fail(void);

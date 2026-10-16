// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include "esp_mac.h"

bool &unit_test_esp_read_mac_fail(void)
{
    static bool fail = false;
    return fail;
}

esp_err_t esp_read_mac(uint8_t *mac, int type)
{
    (void)type;
    if (unit_test_esp_read_mac_fail()) {
        return ESP_FAIL;
    }
    if (mac == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    mac[0] = 0xAA;
    mac[1] = 0xBB;
    mac[2] = 0xCC;
    mac[3] = 0x11;
    mac[4] = 0x22;
    mac[5] = 0x33;
    return ESP_OK;
}

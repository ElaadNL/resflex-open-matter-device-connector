// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>

typedef int esp_err_t;

#define ESP_OK 0
#define ESP_FAIL 0x101
#define ESP_ERR_INVALID_ARG 0x102
#define ESP_ERR_NOT_SUPPORTED 0x106
#define ESP_ERR_NOT_FOUND 0x105

#define ESP_ERROR_CHECK(x)       \
    do {                         \
        esp_err_t _err_rc = (x); \
        (void)_err_rc;           \
    } while (0)

inline const char *esp_err_to_name(int err)
{
    switch (err) {
    case ESP_OK:
        return "ESP_OK";
    case ESP_ERR_INVALID_ARG:
        return "ESP_ERR_INVALID_ARG";
    case ESP_ERR_NOT_SUPPORTED:
        return "ESP_ERR_NOT_SUPPORTED";
    default:
        return "UNKNOWN";
    }
}

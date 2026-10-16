// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <atomic>
#include <string>

#include "esp_err.h"
#include "esp_log.h"

namespace CT {
namespace Charger {

enum class ChargerStatus_t {
    INIT,
    AVAILABLE,
    PREPARING,
    CHARGING,
    SUSPENDED_EVSE,
    SUSPENDED_EV,
    FINISHING,
    RESERVED,
    UNAVAILABLE,
    FAULTED,
};

// enum for auth type
enum class AuthType_t {
    AUTH_TYPE_NONE = 0,
    AUTH_TYPE_NFC_CARD,
    AUTH_TYPE_NFC_W_TIME,
    AUTH_TYPE_MATTER_CTRL,
    AUTH_TYPE_MAX
};

} // namespace Charger
} // namespace CT

// #define ENABLE_CT_LOG

#ifdef ENABLE_CT_LOG
#define PRINTF_DEBUG(format, ...)                                                                            \
    ESP_LOGI("CT", "%s(%d)\n" LOG_COLOR(LOG_COLOR_CYAN) format LOG_RESET_COLOR "\n", __FUNCTION__, __LINE__, \
             ##__VA_ARGS__)
#define DEBUG_CHECKPOINT(format, ...) \
    printf("%s\n" LOG_COLOR(LOG_COLOR_GREEN) format LOG_RESET_COLOR "\n", __FUNCTION__, ##__VA_ARGS__)
#else
#define PRINTF_DEBUG(format, ...)
#define DEBUG_CHECKPOINT(format, ...)
#endif // ENABLE_CT_LOG

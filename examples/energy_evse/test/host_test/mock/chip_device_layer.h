// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "chip_support.h"

#include <cstring>
#include <string>

#ifndef CHIP_DEVICE_CONFIG_DEVICE_VENDOR_ID
#define CHIP_DEVICE_CONFIG_DEVICE_VENDOR_ID 0xFFF1
#endif

#ifndef CHIP_DEVICE_CONFIG_DEVICE_PRODUCT_ID
#define CHIP_DEVICE_CONFIG_DEVICE_PRODUCT_ID 0x8000
#endif

namespace chip {
namespace DeviceLayer {

class ConfigurationManager {
public:
    static constexpr size_t kMaxLocationLength = 2;

    static ConfigurationManager &GetInstance()
    {
        static ConfigurationManager inst;
        return inst;
    }

    void SetCountryCodeForTest(const char *code) { country_code_ = code ? code : ""; }

    CHIP_ERROR GetCountryCode(char *buf, size_t bufSize, size_t &codeLen)
    {
        if (buf == nullptr || bufSize == 0) {
            return CHIP_ERROR_INVALID_ARGUMENT;
        }
        codeLen = country_code_.size();
        if (codeLen >= bufSize) {
            return CHIP_ERROR_BUFFER_TOO_SMALL;
        }
        std::memcpy(buf, country_code_.c_str(), codeLen);
        buf[codeLen] = '\0';
        return CHIP_NO_ERROR;
    }

private:
    std::string country_code_;
};

inline ConfigurationManager &ConfigurationMgr()
{
    return ConfigurationManager::GetInstance();
}

} // namespace DeviceLayer
} // namespace chip

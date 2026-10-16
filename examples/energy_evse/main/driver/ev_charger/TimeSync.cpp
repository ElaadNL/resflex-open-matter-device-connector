/*
 *
 *    Copyright (c) 2024 Project CHIP Authors
 *    All rights reserved.
 *    SPDX-FileCopyrightText: ElaadNL
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "helpers.h"

#ifdef UNIT_TEST
#include "chip_support.h"
#include <iterator>
#include "esp_sntp.h"
#include "nvs.h"
#else
#include "nvs_flash.h"
#include <esp_sntp.h>
#include <lib/support/logging/CHIPLogging.h>
#include <lib/support/TimeUtils.h>
#endif

#include <array>
#include <string>

#include "TimeSync.h"

static constexpr time_t kMinValidTimeStampEpoch = 1704067200; // 1 Jan 2019
#ifndef UNIT_TEST
static constexpr uint32_t kMilliSecondsInADay = 24 * 60 * 60 * 1000;
#endif

namespace {

constexpr uint8_t kMaxNtpServerStringSize = 128;

/** SNTP retains hostname pointers; lifetime must span the process. */
std::array<std::string, chip::Esp32TimeSync::kMaxConfiguredNtpServers> &SntpServerNameStorage()
{
    static std::array<std::string, chip::Esp32TimeSync::kMaxConfiguredNtpServers> sNames;
    return sNames;
}

CHIP_ERROR GetLocalTimeString(std::string &localTime)
{
    struct tm timeinfo;
    std::array<char, 64> strftimeBuf = {};
    time_t now;
    time(&now);
    localtime_r(&now, &timeinfo);
    if (strftime(strftimeBuf.data(), strftimeBuf.size(), "%Y-%m-%dT%H:%M:%S%z", &timeinfo) == 0) {
        PRINTF_DEBUG("Buffer too small");
        return CHIP_ERROR_BUFFER_TOO_SMALL;
    }
    localTime.assign(strftimeBuf.data());
    localTime += ", DST: ";
    localTime += timeinfo.tm_isdst ? "Yes" : "No";
    return CHIP_NO_ERROR;
}

bool ValidateTime()
{
    time_t now;
    time(&now);
    return (now > kMinValidTimeStampEpoch);
}

CHIP_ERROR PrintCurrentTime()
{
    std::string localTime;
    ReturnErrorOnFailure(GetLocalTimeString(localTime));
    if (!ValidateTime()) {
        PRINTF_DEBUG("Time not synchronised yet.");
        return CHIP_ERROR_INCORRECT_STATE;
    }
    PRINTF_DEBUG("The current time is: %s.", localTime.c_str());
    return CHIP_NO_ERROR;
}

void TimeSyncCallback(struct timeval *tv)
{
    if (PrintCurrentTime() != CHIP_NO_ERROR) {
        DEBUG_CHECKPOINT("SNTP time is not synchronised.");
        return;
    }

    nvs_handle_t my_handle;
    esp_err_t err;

    // 1. Open NVS namespace "storage"
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        PRINTF_DEBUG("Error opening NVS handle: %s", esp_err_to_name(err));
        return;
    }

    // 2. Save the seconds and microseconds (persistent time)
    // Using int64_t for seconds ensures future-proofing (Year 2038)
    nvs_set_i64(my_handle, "sync_sec", (int64_t)tv->tv_sec);
    nvs_set_i32(my_handle, "sync_usec", (int32_t)tv->tv_usec);

    // 3. Commit the changes to flash
    err = nvs_commit(my_handle);
    if (err == ESP_OK) {
        PRINTF_DEBUG("Time synced and saved to NVS: %lld", (int64_t)tv->tv_sec);
    }

    // 4. Close the handle
    nvs_close(my_handle);

    DEBUG_CHECKPOINT("SNTP Time Synchronized!");
}

} // anonymous namespace

namespace chip {
namespace Esp32TimeSync {
void Init(const char *const *serverNames, size_t serverCount, const uint16_t syncIntervalDay)
{
    if (!syncIntervalDay) {
        PRINTF_DEBUG("Invalid SNTP synchronization time interval.");
        return;
    }
    if (serverNames == nullptr || serverCount == 0) {
        PRINTF_DEBUG("No NTP servers configured; skipping SNTP init.");
        return;
    }

    auto &storedNames = SntpServerNameStorage();
    size_t configured = 0;
    for (size_t i = 0; i < serverCount && configured < storedNames.size(); ++i) {
        if (serverNames[i] == nullptr || serverNames[i][0] == '\0') {
            continue;
        }
        storedNames[configured] = serverNames[i];
        if (storedNames[configured].size() > kMaxNtpServerStringSize) {
            storedNames[configured].resize(kMaxNtpServerStringSize);
        }
        ++configured;
    }

    if (configured == 0) {
        PRINTF_DEBUG("No NTP servers configured; skipping SNTP init.");
        return;
    }

    if (esp_sntp_enabled()) {
        PRINTF_DEBUG("SNTP already initialized.");
#ifdef UNIT_TEST
        return;
#endif
    }

    PRINTF_DEBUG("Initializing SNTP with %u configured server(s) (plain SNTP is unauthenticated)",
                 static_cast<unsigned>(configured));
    for (size_t i = 0; i < configured; ++i) {
        PRINTF_DEBUG("  NTP[%u]: %s", static_cast<unsigned>(i), storedNames[i].c_str());
    }

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    for (size_t i = 0; i < configured; ++i) {
        esp_sntp_setservername(static_cast<uint8_t>(i), storedNames[i].c_str());
    }
#ifndef UNIT_TEST
    esp_sntp_set_sync_interval(static_cast<uint32_t>(kMilliSecondsInADay) * syncIntervalDay);
#endif
    sntp_set_time_sync_notification_cb(TimeSyncCallback);
    esp_sntp_init();
}
} // namespace Esp32TimeSync
} // namespace chip

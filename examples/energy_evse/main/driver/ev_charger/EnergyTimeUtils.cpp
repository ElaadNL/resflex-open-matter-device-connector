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

#include <time.h>
#include <sys/time.h>

#include "helpers.h"

#ifdef UNIT_TEST
#include "EnergyTimeUtils.h"
#else
#include <EnergyTimeUtils.h>
#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/cluster-objects.h>
#include <app/EventLogging.h>
#endif

using namespace chip;
#ifndef UNIT_TEST
using namespace chip::app;
using namespace chip::app::DataModel;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::EnergyEvse;
using namespace chip::app::Clusters::EnergyEvse::Attributes;

using chip::app::LogEvent;
using chip::Protocols::InteractionModel::Status;
#else
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::EnergyEvse;
#endif

namespace chip {
namespace app {
namespace Clusters {
namespace DeviceEnergyManagement {

/**
 * @brief   Helper function to get current timestamp in Epoch format
 *
 * @param[out]   chipEpoch reference to hold return timestamp. Set to 0 if an error occurs.
 */
CHIP_ERROR GetEpochTS(uint32_t &chipEpoch)
{
    chipEpoch = 0;

    // 1. Check the underlying POSIX system time (standard C)
    // time_t posix_now;
    // time(&posix_now);
    // PRINTF_DEBUG("DEBUG: Standard POSIX time(NULL): %ld", (long)posix_now);

    // 2. Check what the Matter System Clock is reporting
    System::Clock::Milliseconds64 cTMs;
    CHIP_ERROR err = System::SystemClock().GetClock_RealTimeMS(cTMs);

    VerifyOrDie(err != CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE);

    if (err != CHIP_NO_ERROR) {
        PRINTF_DEBUG("Unable to get current time - err:%d", err);
        return err;
    }

    // 3. Compare Unix Epoch from SDK vs Expected
#ifdef UNIT_TEST
    const auto unixEpoch = static_cast<uint32_t>(cTMs / 1000);
#else
    auto unixEpoch = std::chrono::duration_cast<System::Clock::Seconds32>(cTMs).count();
#endif
    PRINTF_DEBUG("DEBUG: Matter SDK Unix Epoch: %u", (uint32_t)unixEpoch);

    // 4. Check the Conversion to Chip Epoch (Matter Epoch)
    if (!UnixEpochToChipEpochTime(unixEpoch, chipEpoch)) {
        PRINTF_DEBUG("DEBUG: Conversion FAILED for Unix: %u", (uint32_t)unixEpoch);
        return CHIP_ERROR_INCORRECT_STATE;
    }

    PRINTF_DEBUG("DEBUG: Final Chip/Matter Epoch: %u", chipEpoch);

    return CHIP_NO_ERROR;
}

/**
 * @brief   Helper function to get current timestamp and work out the day of week
 *
 * NOTE that the time_t is converted using localtime to provide the timestamp
 * in local time. If this is not supported on some platforms an alternative
 * implementation may be required.
 *
 * @param   unixEpoch (as time_t)
 *
 * @return  bitmap value for day of week as defined by EnergyEvse::TargetDayOfWeekBitmap. Note
 *          only one bit will be set for the day of the week.
 */
BitMask<EnergyEvse::TargetDayOfWeekBitmap> GetLocalDayOfWeekFromUnixEpoch(time_t unixEpoch)
{
    // Define a timezone structure and initialize it to the local timezone
    // This will capture any daylight saving time changes
    struct tm local_time;
    localtime_r(&unixEpoch, &local_time);

    // Get the day of the week (0 = Sunday, 1 = Monday, ..., 6 = Saturday)
    auto dayOfWeek = static_cast<uint8_t>(local_time.tm_wday);

    // Calculate the bitmap value based on the day of the week. Note that the value in bitmap
    // maps directly to the definition in EnergyEvse::TargetDayOfWeekBitmap.
    const auto bitmap = static_cast<uint8_t>(1 << dayOfWeek);

    return BitMask<TargetDayOfWeekBitmap>(bitmap);
}
/**
 * @brief   Helper function to get current timestamp and work out the day of week based on localtime
 *
 * @return  bitmap value for day of week as defined by EnergyEvse::TargetDayOfWeekBitmap. Note
 *          only one bit will be set for the current day.
 */
CHIP_ERROR GetLocalDayOfWeekNow(BitMask<EnergyEvse::TargetDayOfWeekBitmap> &dayOfWeekMap)
{
    System::Clock::Milliseconds64 cTMs;
    CHIP_ERROR err = System::SystemClock().GetClock_RealTimeMS(cTMs);
    if (err != CHIP_NO_ERROR) {
        PRINTF_DEBUG("Unable to get current time - err:%d", err);
        return err;
    }
#ifdef UNIT_TEST
    const time_t unixEpoch = static_cast<time_t>(cTMs / 1000);
#else
    time_t unixEpoch = std::chrono::duration_cast<System::Clock::Seconds32>(cTMs).count();
#endif

    dayOfWeekMap = GetLocalDayOfWeekFromUnixEpoch(unixEpoch);

    return CHIP_NO_ERROR;
}

/**
 * @brief   Helper function to get current timestamp and work out the current number of minutes
 *          past midnight based on localtime
 *
 * @param   reference to hold the number of minutes past midnight
 */
CHIP_ERROR GetMinutesPastMidnight(uint16_t &minutesPastMidnight)
{
    System::Clock::Milliseconds64 cTMs;
    CHIP_ERROR err = System::SystemClock().GetClock_RealTimeMS(cTMs);
    if (err != CHIP_NO_ERROR) {
        PRINTF_DEBUG("Unable to get current time - err:%d", err);
        return err;
    }
#ifdef UNIT_TEST
    const time_t unixEpoch = static_cast<time_t>(cTMs / 1000);
#else
    time_t unixEpoch = std::chrono::duration_cast<chip::System::Clock::Seconds32>(cTMs).count();
#endif

    // Define a timezone structure and initialize it to the local timezone
    // This will capture any daylight saving time changes
    struct tm local_time;
    localtime_r(&unixEpoch, &local_time);

    minutesPastMidnight = static_cast<uint16_t>((local_time.tm_hour * 60) + local_time.tm_min);

    return CHIP_NO_ERROR;
}

} // namespace DeviceEnergyManagement
} // namespace Clusters
} // namespace app
} // namespace chip

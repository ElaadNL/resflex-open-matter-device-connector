// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include "get_readable_time.h"

#include <cstdio>
#include <ctime>

namespace {
// Matter spec: CHIP epoch starts at 2000-01-01 00:00:00 UTC (same as chip::kChipEpochSecondsSinceUnixEpoch).
constexpr uint32_t kChipEpochSecondsSinceUnixEpoch = 946684800u;
} // namespace

void GetReadableTime(uint32_t matterEpoch_s, char *outStr, size_t outSize)
{
    if (matterEpoch_s == 0) {
        snprintf(outStr, outSize, "NOT SET");
        return;
    }

    time_t unixTime = static_cast<time_t>(matterEpoch_s + kChipEpochSecondsSinceUnixEpoch);

    struct tm timeInfo;
    // FIX: Changed from localtime_r to gmtime_r to enforce UTC
    gmtime_r(&unixTime, &timeInfo);

    strftime(outStr, outSize, "%d/%m/%Y %H:%M", &timeInfo);
}

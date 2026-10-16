// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include "EnergyTimeUtils.h"

#include "unity.h"

#include <ctime>

using namespace chip;
using namespace chip::app::Clusters::DeviceEnergyManagement;
using namespace chip::app::Clusters::EnergyEvse;

static void test_get_epoch_ts_converts(void)
{
    chip::System::SystemClockInstance().SetRealTimeMsForTest(946684800000LL); // 2000-01-01 UTC
    uint32_t chipEpoch = 99;
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, GetEpochTS(chipEpoch));
    TEST_ASSERT_EQUAL_UINT32(0, chipEpoch);
}

static void test_get_local_day_of_week_sunday(void)
{
    struct tm tm_val {
    };
    tm_val.tm_year = 124;
    tm_val.tm_mon = 0;
    tm_val.tm_mday = 7;
    tm_val.tm_hour = 12;
    tm_val.tm_isdst = -1;
    const time_t t = mktime(&tm_val);
    const auto day = GetLocalDayOfWeekFromUnixEpoch(t);
    TEST_ASSERT_EQUAL_UINT8(0x01, day.Raw());
}

static void test_get_minutes_past_midnight(void)
{
    const time_t unixEpoch = 1'700'000'000;
    chip::System::SystemClockInstance().SetRealTimeMsForTest(static_cast<int64_t>(unixEpoch) * 1000);
    struct tm local_time;
    localtime_r(&unixEpoch, &local_time);
    const uint16_t expected = static_cast<uint16_t>((local_time.tm_hour * 60) + local_time.tm_min);

    uint16_t minutes = 0;
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, GetMinutesPastMidnight(minutes));
    TEST_ASSERT_EQUAL_UINT16(expected, minutes);
}

void run_test_energy_time_utils_tests(void)
{
    RUN_TEST(test_get_epoch_ts_converts);
    RUN_TEST(test_get_local_day_of_week_sunday);
    RUN_TEST(test_get_minutes_past_midnight);
}

// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include "unity.h"

#include "charger_session_math.h"

static void test_session_energy_delivered_mWh(void)
{
    uint32_t delivered = CT::Charger::session_energy_delivered_mWh(12500, static_cast<int64_t>(2500));
    TEST_ASSERT_EQUAL_UINT32(10000u, delivered);
}

static void test_session_time_elapsed_sec_from_monotonic_us(void)
{
    int64_t elapsed = CT::Charger::session_time_elapsed_sec_from_monotonic_us(10500000LL, 2500000LL);
    TEST_ASSERT_EQUAL_INT64(8LL, elapsed);
}

static void test_charger_thread_tick_aligns_interval(void)
{
    TEST_ASSERT_TRUE(CT::Charger::charger_thread_tick_aligns_interval(0u, 1u, 200u));
    TEST_ASSERT_FALSE(CT::Charger::charger_thread_tick_aligns_interval(4u, 1u, 200u));
    TEST_ASSERT_TRUE(CT::Charger::charger_thread_tick_aligns_interval(5u, 1u, 200u));
}

void run_test_charger_session_math_tests(void)
{
    RUN_TEST(test_session_energy_delivered_mWh);
    RUN_TEST(test_session_time_elapsed_sec_from_monotonic_us);
    RUN_TEST(test_charger_thread_tick_aligns_interval);
}

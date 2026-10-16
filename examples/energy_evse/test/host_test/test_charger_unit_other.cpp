// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include "unity.h"

#include "calculate_energy.h"
#include "get_readable_time.h"

static void test_calculate_energy_standard_value(void)
{
    float result = calculate_energy(10.0f, 230.0f, 1.0f);
    TEST_ASSERT_EQUAL_FLOAT(2.3f, result);
}

static void test_calculate_energy_zero_amps(void)
{
    float result = calculate_energy(0.0f, 230.0f, 1.0f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, result);
}

static void test_epoch_zero(void)
{
    char buf[128];
    GetReadableTime(0, buf, sizeof buf);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("NOT SET", buf, "Expected NOT SET");
}

static void test_epoch_86400(void)
{
    char buf[128];
    GetReadableTime(86400u, buf, sizeof buf);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("02/01/2000 00:00", buf, "Expected 02/01/2000 00:00");
}

void run_test_charger_unit_other_tests(void)
{
    RUN_TEST(test_calculate_energy_standard_value);
    RUN_TEST(test_calculate_energy_zero_amps);
    RUN_TEST(test_epoch_zero);
    RUN_TEST(test_epoch_86400);
}

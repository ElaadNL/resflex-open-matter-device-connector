// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include "matterManager.h"

#include "unity.h"

using CT::Charger::MatterManager;

static void test_support_discharging_v2x_false(void)
{
    TEST_ASSERT_FALSE(CT::Charger::kSupportDischargingV2x);
}

static void test_get_instance_returns_singleton(void)
{
    MatterManager &first = MatterManager::GetInstance();
    MatterManager &second = MatterManager::GetInstance();
    TEST_ASSERT_EQUAL_PTR(&first, &second);
}

void run_test_matter_manager_header_tests(void)
{
    RUN_TEST(test_support_discharging_v2x_false);
    RUN_TEST(test_get_instance_returns_singleton);
}

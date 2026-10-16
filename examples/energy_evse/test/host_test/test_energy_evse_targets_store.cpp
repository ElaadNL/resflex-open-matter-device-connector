// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include "unit_test_exports.h"

#include "unity.h"

static void test_target_skipped_when_in_past_and_not_allowed(void)
{
    TEST_ASSERT_TRUE(EnergyEvse_TargetSkippedAsPast(30, 60, false));
}

void run_test_energy_evse_targets_store_tests(void)
{
    RUN_TEST(test_target_skipped_when_in_past_and_not_allowed);
}

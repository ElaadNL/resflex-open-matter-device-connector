// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include "chargerManager.h"
#include "matterManager.h"

#include "unity.h"

using CT::Charger::ChargerManager;
using CT::Charger::MatterManager;

static void test_set_matter_delegate_energy_evse(void)
{
    ChargerManager &mgr = ChargerManager::Controller();
    MatterManager &mm = MatterManager::GetInstance();
    mgr.resetForTest();

    TEST_ASSERT_NULL(mgr.EE_dg);
    mgr.SetMatterDelegateEnergyEvse(&mm.EE_dg);
    TEST_ASSERT_EQUAL_PTR(&mm.EE_dg, mgr.EE_dg);
}

static void test_charger_manager_thread_ticks(void)
{
    TEST_ASSERT_EQUAL(200, CHARGER_MGR_THREAD_TICKS);
}

static void test_charger_status_enum_values(void)
{
    TEST_ASSERT_TRUE(static_cast<int>(CT::Charger::ChargerStatus_t::AVAILABLE) >= 0);
    TEST_ASSERT_TRUE(static_cast<int>(CT::Charger::ChargerStatus_t::FAULTED) >= 0);
}

void run_test_charger_manager_header_tests(void)
{
    RUN_TEST(test_set_matter_delegate_energy_evse);
    RUN_TEST(test_charger_manager_thread_ticks);
    RUN_TEST(test_charger_status_enum_values);
}

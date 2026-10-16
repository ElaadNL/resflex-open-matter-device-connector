// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include "EnergyEvseDelegateImpl.h"

#include "unity.h"

using namespace chip::app::Clusters::EnergyEvse;

static void test_all_target_days_mask(void)
{
    TEST_ASSERT_EQUAL_UINT8(0x7f, kAllTargetDaysMask);
}

static void test_evse_session_set_endpoint_id(void)
{
    EvseSession session;
    session.SetEndpointId(9);
    EnergyEvseDelegate delegate;
    delegate.mSession.SetEndpointId(10);
    TEST_ASSERT_NOT_NULL(delegate.GetEvseTargetsDelegate());
}

void run_test_energy_evse_delegate_impl_header_tests(void)
{
    RUN_TEST(test_all_target_days_mask);
    RUN_TEST(test_evse_session_set_endpoint_id);
}

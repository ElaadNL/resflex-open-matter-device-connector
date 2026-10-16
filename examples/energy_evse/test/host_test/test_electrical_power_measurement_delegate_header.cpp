// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include "ElectricalPowerMeasurementDelegate.h"

#include "unity.h"

using namespace chip::app::Clusters::ElectricalPowerMeasurement;

static void test_power_mode_default_ac(void)
{
    ElectricalPowerMeasurementDelegate delegate;
    TEST_ASSERT_EQUAL(static_cast<int>(PowerModeEnum::kAc), static_cast<int>(delegate.GetPowerMode()));
}

static void test_get_endpoint_id_after_setup(void)
{
    ElectricalPowerMeasurementDelegate delegate;
    delegate.SetupDelegate(11);
    TEST_ASSERT_EQUAL(11, delegate.GetEndpointId());
}

void run_test_electrical_power_measurement_delegate_header_tests(void)
{
    RUN_TEST(test_power_mode_default_ac);
    RUN_TEST(test_get_endpoint_id_after_setup);
}

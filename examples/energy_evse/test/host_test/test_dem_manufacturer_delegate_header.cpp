// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include <DEMManufacturerDelegate.h>

#include "unity.h"

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters::DeviceEnergyManagement;

namespace {

class TestDemManufacturerDelegate : public DEMManufacturerDelegate {
public:
    int64_t GetApproxEnergyDuringSession() override { return 42; }
};

} // namespace

static void test_pure_virtual_energy_session(void)
{
    TestDemManufacturerDelegate delegate;
    TEST_ASSERT_EQUAL(42, delegate.GetApproxEnergyDuringSession());
}

static void test_default_handlers_return_ok(void)
{
    TestDemManufacturerDelegate delegate;
    constexpr AdjustmentCauseEnum kCause = AdjustmentCauseEnum::kLocalOptimization;
    constexpr CauseEnum kCompletionCause = CauseEnum::kNormalCompletion;

    Structs::SlotAdjustmentStruct::DecodableType slot{};
    slot.slotIndex = 1;
    DataModel::DecodableList<Structs::SlotAdjustmentStruct::DecodableType> slotAdjustments{slot};

    Structs::ConstraintsStruct::DecodableType constraint{};
    constraint.power = 5000;
    DataModel::DecodableList<Structs::ConstraintsStruct::DecodableType> constraints{constraint};

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.HandleDeviceEnergyManagementPowerAdjustRequest(1000, 60, kCause));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.HandleDeviceEnergyManagementPowerAdjustCompletion());
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.HandleDeviceEnergyManagementCancelPowerAdjustRequest(kCompletionCause));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.HandleDeviceEnergyManagementStartTimeAdjustRequest(12345, kCause));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.HandleDeviceEnergyManagementPauseRequest(30, kCause));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.HandleDeviceEnergyManagementPauseCompletion());
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.HandleDeviceEnergyManagementCancelPauseRequest(kCompletionCause));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.HandleDeviceEnergyManagementCancelRequest());
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.HandleModifyForecastRequest(7, slotAdjustments, kCause));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.RequestConstraintBasedForecast(constraints, kCause));
}

static void test_virtual_destructor_via_base_pointer(void)
{
    auto *delegate = new TestDemManufacturerDelegate();
    DEMManufacturerDelegate *base = delegate;
    delete base;
}

void run_test_dem_manufacturer_delegate_header_tests(void)
{
    RUN_TEST(test_pure_virtual_energy_session);
    RUN_TEST(test_default_handlers_return_ok);
    RUN_TEST(test_virtual_destructor_via_base_pointer);
}

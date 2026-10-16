// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include "DeviceEnergyManagementDelegateImpl.h"
#include "chip_event_logging.h"
#include "chip_system_layer.h"
#include "EnergyTimeUtils.h"
#include "matterManager.h"

#include "unity.h"

#include <vector>

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::DeviceEnergyManagement;
using namespace chip::Protocols::InteractionModel;
using CT::Charger::MatterManager;

namespace {

class TestDemManufacturerDelegate : public DEMManufacturerDelegate {
public:
    int64_t session_energy = 5000;

    CHIP_ERROR power_adjust_request_err = CHIP_NO_ERROR;
    CHIP_ERROR start_time_adjust_err = CHIP_NO_ERROR;
    CHIP_ERROR pause_request_err = CHIP_NO_ERROR;
    CHIP_ERROR modify_forecast_err = CHIP_NO_ERROR;
    CHIP_ERROR constraint_forecast_err = CHIP_NO_ERROR;
    CHIP_ERROR cancel_power_adjust_err = CHIP_NO_ERROR;
    CHIP_ERROR cancel_pause_err = CHIP_NO_ERROR;
    CHIP_ERROR cancel_request_err = CHIP_NO_ERROR;

    int64_t GetApproxEnergyDuringSession() override { return session_energy; }

    CHIP_ERROR HandleDeviceEnergyManagementPowerAdjustRequest(int64_t, uint32_t, AdjustmentCauseEnum) override
    {
        return power_adjust_request_err;
    }

    CHIP_ERROR HandleDeviceEnergyManagementStartTimeAdjustRequest(uint32_t, AdjustmentCauseEnum) override
    {
        return start_time_adjust_err;
    }

    CHIP_ERROR HandleDeviceEnergyManagementPauseRequest(uint32_t, AdjustmentCauseEnum) override
    {
        return pause_request_err;
    }

    CHIP_ERROR HandleModifyForecastRequest(
        uint32_t, const DataModel::DecodableList<Structs::SlotAdjustmentStruct::DecodableType> &,
        AdjustmentCauseEnum) override
    {
        return modify_forecast_err;
    }

    CHIP_ERROR RequestConstraintBasedForecast(
        const DataModel::DecodableList<Structs::ConstraintsStruct::DecodableType> &, AdjustmentCauseEnum) override
    {
        return constraint_forecast_err;
    }

    CHIP_ERROR HandleDeviceEnergyManagementCancelPowerAdjustRequest(CauseEnum) override
    {
        return cancel_power_adjust_err;
    }

    CHIP_ERROR HandleDeviceEnergyManagementCancelPauseRequest(CauseEnum) override { return cancel_pause_err; }

    CHIP_ERROR HandleDeviceEnergyManagementCancelRequest() override { return cancel_request_err; }
};

void reset_test_harness(void)
{
    DeviceLayer::SystemLayer().ResetForTest();
    unit_test_log_event_fail(false);
    MatterManager::GetInstance().isConnected = true;
    MatterManager::ReportAttributeCallCount() = 0;
    System::SystemClockInstance().SetRealTimeMsForTest(1'700'000'000LL * 1000);
}

void set_power_capability(DeviceEnergyManagementDelegate &delegate)
{
    Nullable<Structs::PowerAdjustCapabilityStruct::Type> capability;
    capability.SetNonNull({});
    delegate.SetPowerAdjustmentCapability(capability);
}

void set_forecast(DeviceEnergyManagementDelegate &delegate, uint32_t forecast_id = 10)
{
    Structs::ForecastStruct::Type forecast{};
    forecast.forecastID = forecast_id;
    forecast.startTime = 1'000;
    forecast.endTime = 2'000;
    forecast.forecastUpdateReason = ForecastUpdateReasonEnum::kInternalOptimization;

    Nullable<Structs::ForecastStruct::Type> wrapped;
    wrapped.SetNonNull(forecast);
    delegate.SetForecast(wrapped);
}

} // namespace

static void test_constructor_defaults(void)
{
    reset_test_harness();
    DeviceEnergyManagementDelegate delegate;

    TEST_ASSERT_EQUAL(static_cast<int>(ESATypeEnum::kEvse), static_cast<int>(delegate.GetESAType()));
    TEST_ASSERT_FALSE(delegate.GetESACanGenerate());
    TEST_ASSERT_EQUAL(static_cast<int>(ESAStateEnum::kOffline), static_cast<int>(delegate.GetESAState()));
    TEST_ASSERT_EQUAL(0, delegate.GetAbsMinPower());
    TEST_ASSERT_EQUAL(0, delegate.GetAbsMaxPower());
    TEST_ASSERT_EQUAL(static_cast<int>(OptOutStateEnum::kNoOptOut), static_cast<int>(delegate.GetOptOutState()));
}

static void test_setup_delegate_and_has_feature(void)
{
    reset_test_harness();
    DeviceEnergyManagementDelegate delegate;
    Instance instance;
    instance.SetFeatureForTest(Feature::kPowerAdjustment);

    delegate.SetupDelegate(3);
    delegate.SetDeviceEnergyManagementInstance(instance);

    TEST_ASSERT_EQUAL(3, delegate.mEndpointId);
    TEST_ASSERT_EQUAL(1u, delegate.HasFeature(Feature::kPowerAdjustment));
    TEST_ASSERT_EQUAL(0u, delegate.HasFeature(Feature{static_cast<uint32_t>(0)}));
}

static void test_setters_and_getters(void)
{
    reset_test_harness();
    DeviceEnergyManagementDelegate delegate;
    delegate.SetupDelegate(1);

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetESAType(ESATypeEnum::kEvse));
    TEST_ASSERT_EQUAL(CHIP_ERROR_INVALID_ARGUMENT, delegate.SetESAType(ESATypeEnum::kUnknownEnumValue));

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetESACanGenerate(true));
    TEST_ASSERT_TRUE(delegate.GetESACanGenerate());

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetESAState(ESAStateEnum::kOnline));
    TEST_ASSERT_EQUAL(static_cast<int>(ESAStateEnum::kOnline), static_cast<int>(delegate.GetESAState()));
    TEST_ASSERT_EQUAL(CHIP_ERROR_INVALID_ARGUMENT, delegate.SetESAState(ESAStateEnum::kUnknownEnumValue));

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetAbsMinPower(-1000));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetAbsMaxPower(7000));
    TEST_ASSERT_EQUAL(-1000, delegate.GetAbsMinPower());
    TEST_ASSERT_EQUAL(7000, delegate.GetAbsMaxPower());

    set_power_capability(delegate);
    set_forecast(delegate);
    TEST_ASSERT_FALSE(delegate.GetPowerAdjustmentCapability().IsNull());
    TEST_ASSERT_FALSE(delegate.GetForecast().IsNull());
    TEST_ASSERT_TRUE(MatterManager::ReportAttributeCallCount() > 0);
}

static void test_power_adjust_request_local_success(void)
{
    reset_test_harness();
    DeviceEnergyManagementDelegate delegate;
    TestDemManufacturerDelegate manufacturer;
    delegate.SetupDelegate(1);
    set_power_capability(delegate);
    delegate.SetDEMManufacturerDelegate(manufacturer);

    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success),
                      static_cast<int>(delegate.PowerAdjustRequest(1000, 30, AdjustmentCauseEnum::kLocalOptimization)));
    TEST_ASSERT_EQUAL(static_cast<int>(ESAStateEnum::kPowerAdjustActive), static_cast<int>(delegate.GetESAState()));

    DeviceLayer::SystemLayer().FireExpiredTimersForTest();
    TEST_ASSERT_EQUAL(static_cast<int>(ESAStateEnum::kOnline), static_cast<int>(delegate.GetESAState()));
}

static void test_power_adjust_request_grid_and_second_request(void)
{
    reset_test_harness();
    DeviceEnergyManagementDelegate delegate;
    delegate.SetupDelegate(1);
    set_power_capability(delegate);

    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success),
                      static_cast<int>(delegate.PowerAdjustRequest(500, 10, AdjustmentCauseEnum::kGridOptimization)));

    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success),
                      static_cast<int>(delegate.PowerAdjustRequest(600, 5, AdjustmentCauseEnum::kGridOptimization)));
}

static void test_power_adjust_request_bad_cause_and_failures(void)
{
    reset_test_harness();
    DeviceEnergyManagementDelegate delegate;
    TestDemManufacturerDelegate manufacturer;
    delegate.SetupDelegate(1);
    set_power_capability(delegate);
    delegate.SetDEMManufacturerDelegate(manufacturer);

    TEST_ASSERT_EQUAL(static_cast<int>(Status::Failure),
                      static_cast<int>(delegate.PowerAdjustRequest(1, 1, AdjustmentCauseEnum::kUnknownEnumValue)));

    manufacturer.power_adjust_request_err = CHIP_ERROR_INTERNAL;
    TEST_ASSERT_EQUAL(static_cast<int>(Status::Failure),
                      static_cast<int>(delegate.PowerAdjustRequest(1, 1, AdjustmentCauseEnum::kLocalOptimization)));

    manufacturer.power_adjust_request_err = CHIP_NO_ERROR;
    DeviceLayer::SystemLayer().SetStartTimerFailForTest(true);
    TEST_ASSERT_EQUAL(static_cast<int>(Status::Failure),
                      static_cast<int>(delegate.PowerAdjustRequest(1, 1, AdjustmentCauseEnum::kLocalOptimization)));
    DeviceLayer::SystemLayer().SetStartTimerFailForTest(false);

    unit_test_log_event_fail(true);
    TEST_ASSERT_EQUAL(static_cast<int>(Status::Failure),
                      static_cast<int>(delegate.PowerAdjustRequest(1, 1, AdjustmentCauseEnum::kLocalOptimization)));
    unit_test_log_event_fail(false);
}

static void test_cancel_power_adjust_request(void)
{
    reset_test_harness();
    DeviceEnergyManagementDelegate delegate;
    delegate.SetupDelegate(1);
    set_power_capability(delegate);

    delegate.PowerAdjustRequest(100, 60, AdjustmentCauseEnum::kLocalOptimization);
    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success), static_cast<int>(delegate.CancelPowerAdjustRequest()));
    TEST_ASSERT_EQUAL(static_cast<int>(ESAStateEnum::kOnline), static_cast<int>(delegate.GetESAState()));
}

static void test_start_time_adjust_request(void)
{
    reset_test_harness();
    DeviceEnergyManagementDelegate delegate;
    TestDemManufacturerDelegate manufacturer;
    delegate.SetupDelegate(1);
    delegate.SetDEMManufacturerDelegate(manufacturer);

    TEST_ASSERT_EQUAL(static_cast<int>(Status::Failure),
                      static_cast<int>(delegate.StartTimeAdjustRequest(1500, AdjustmentCauseEnum::kLocalOptimization)));

    set_forecast(delegate);

    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success),
                      static_cast<int>(delegate.StartTimeAdjustRequest(1500, AdjustmentCauseEnum::kLocalOptimization)));

    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success),
                      static_cast<int>(delegate.StartTimeAdjustRequest(1600, AdjustmentCauseEnum::kGridOptimization)));
    TEST_ASSERT_EQUAL(12u, delegate.GetForecast().Value().forecastID);

    manufacturer.start_time_adjust_err = CHIP_ERROR_INTERNAL;
    set_forecast(delegate, 20);
    TEST_ASSERT_EQUAL(static_cast<int>(Status::Failure),
                      static_cast<int>(delegate.StartTimeAdjustRequest(1600, AdjustmentCauseEnum::kLocalOptimization)));
}

static void test_pause_and_resume_request(void)
{
    reset_test_harness();
    DeviceEnergyManagementDelegate delegate;
    TestDemManufacturerDelegate manufacturer;
    delegate.SetupDelegate(1);
    set_forecast(delegate);
    delegate.SetDEMManufacturerDelegate(manufacturer);

    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success),
                      static_cast<int>(delegate.PauseRequest(20, AdjustmentCauseEnum::kLocalOptimization)));
    TEST_ASSERT_EQUAL(static_cast<int>(ESAStateEnum::kPaused), static_cast<int>(delegate.GetESAState()));

    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success), static_cast<int>(delegate.ResumeRequest()));
    TEST_ASSERT_EQUAL(static_cast<int>(ESAStateEnum::kOnline), static_cast<int>(delegate.GetESAState()));

    set_forecast(delegate);
    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success),
                      static_cast<int>(delegate.PauseRequest(15, AdjustmentCauseEnum::kGridOptimization)));
    DeviceLayer::SystemLayer().FireExpiredTimersForTest();
    TEST_ASSERT_EQUAL(static_cast<int>(ESAStateEnum::kOnline), static_cast<int>(delegate.GetESAState()));
}

static void test_pause_request_failures(void)
{
    reset_test_harness();
    DeviceEnergyManagementDelegate delegate;
    TestDemManufacturerDelegate manufacturer;
    delegate.SetupDelegate(1);
    set_forecast(delegate);
    delegate.SetDEMManufacturerDelegate(manufacturer);

    manufacturer.pause_request_err = CHIP_ERROR_INTERNAL;
    TEST_ASSERT_EQUAL(static_cast<int>(Status::Failure),
                      static_cast<int>(delegate.PauseRequest(5, AdjustmentCauseEnum::kLocalOptimization)));

    manufacturer.pause_request_err = CHIP_NO_ERROR;
    DeviceLayer::SystemLayer().SetStartTimerFailForTest(true);
    TEST_ASSERT_EQUAL(static_cast<int>(Status::Failure),
                      static_cast<int>(delegate.PauseRequest(5, AdjustmentCauseEnum::kLocalOptimization)));
}

static void test_modify_and_constraint_forecast_requests(void)
{
    reset_test_harness();
    DeviceEnergyManagementDelegate delegate;
    TestDemManufacturerDelegate manufacturer;
    delegate.SetupDelegate(1);
    set_forecast(delegate, 3);
    delegate.SetDEMManufacturerDelegate(manufacturer);

    DataModel::DecodableList<Structs::SlotAdjustmentStruct::DecodableType> slots{};
    TEST_ASSERT_EQUAL(
        static_cast<int>(Status::Failure),
        static_cast<int>(delegate.ModifyForecastRequest(99, slots, AdjustmentCauseEnum::kLocalOptimization)));

    TEST_ASSERT_EQUAL(
        static_cast<int>(Status::Success),
        static_cast<int>(delegate.ModifyForecastRequest(3, slots, AdjustmentCauseEnum::kGridOptimization)));

    DataModel::DecodableList<Structs::ConstraintsStruct::DecodableType> constraints{};
    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success),
                      static_cast<int>(delegate.RequestConstraintBasedForecast(
                          constraints, AdjustmentCauseEnum::kLocalOptimization)));

    manufacturer.modify_forecast_err = CHIP_ERROR_INTERNAL;
    TEST_ASSERT_EQUAL(
        static_cast<int>(Status::Failure),
        static_cast<int>(delegate.ModifyForecastRequest(4, slots, AdjustmentCauseEnum::kLocalOptimization)));
}

static void test_cancel_request(void)
{
    reset_test_harness();
    DeviceEnergyManagementDelegate delegate;
    TestDemManufacturerDelegate manufacturer;
    delegate.SetupDelegate(1);
    set_forecast(delegate);
    delegate.SetDEMManufacturerDelegate(manufacturer);

    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success), static_cast<int>(delegate.CancelRequest()));

    manufacturer.cancel_request_err = CHIP_ERROR_INTERNAL;
    TEST_ASSERT_EQUAL(static_cast<int>(Status::Failure), static_cast<int>(delegate.CancelRequest()));
}

static void test_set_opt_out_state_cancels_adjustments(void)
{
    reset_test_harness();
    DeviceEnergyManagementDelegate delegate;
    delegate.SetupDelegate(1);
    set_power_capability(delegate);
    set_forecast(delegate);

    delegate.PowerAdjustRequest(100, 60, AdjustmentCauseEnum::kLocalOptimization);
    delegate.PauseRequest(30, AdjustmentCauseEnum::kLocalOptimization);

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetOptOutState(OptOutStateEnum::kLocalOptOut));
    TEST_ASSERT_EQUAL(static_cast<int>(ESAStateEnum::kOnline), static_cast<int>(delegate.GetESAState()));

    delegate.PowerAdjustRequest(100, 60, AdjustmentCauseEnum::kGridOptimization);
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetOptOutState(OptOutStateEnum::kGridOptOut));

    delegate.PowerAdjustRequest(100, 60, AdjustmentCauseEnum::kLocalOptimization);
    delegate.SetOptOutState(OptOutStateEnum::kGridOptOut);
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetOptOutState(OptOutStateEnum::kOptOut));
}

static void test_normalize_forecast_bad_reason(void)
{
    reset_test_harness();
    DeviceEnergyManagementDelegate delegate;
    delegate.SetupDelegate(1);

    Structs::ForecastStruct::Type forecast{};
    forecast.forecastUpdateReason = ForecastUpdateReasonEnum::kUnknownEnumValue;
    Nullable<Structs::ForecastStruct::Type> wrapped;
    wrapped.SetNonNull(forecast);
    delegate.SetForecast(wrapped);

    TEST_ASSERT_EQUAL(CHIP_ERROR_BAD_REQUEST, delegate.SetOptOutState(OptOutStateEnum::kNoOptOut));
}

static void test_power_adjust_get_epoch_failure(void)
{
    reset_test_harness();
    DeviceEnergyManagementDelegate delegate;
    delegate.SetupDelegate(1);
    set_power_capability(delegate);

    System::SystemClockInstance().SetRealTimeMsForTest(0);
    TEST_ASSERT_EQUAL(static_cast<int>(Status::Failure),
                      static_cast<int>(delegate.PowerAdjustRequest(100, 30, AdjustmentCauseEnum::kLocalOptimization)));
    System::SystemClockInstance().SetRealTimeMsForTest(1'700'000'000LL * 1000);
}

static void test_cancel_power_adjust_manufacturer_and_event_failures(void)
{
    reset_test_harness();
    DeviceEnergyManagementDelegate delegate;
    TestDemManufacturerDelegate manufacturer;
    delegate.SetupDelegate(1);
    set_power_capability(delegate);
    delegate.SetDEMManufacturerDelegate(manufacturer);

    delegate.PowerAdjustRequest(100, 60, AdjustmentCauseEnum::kLocalOptimization);

    manufacturer.cancel_power_adjust_err = CHIP_ERROR_INTERNAL;
    TEST_ASSERT_EQUAL(static_cast<int>(Status::Failure), static_cast<int>(delegate.CancelPowerAdjustRequest()));
    manufacturer.cancel_power_adjust_err = CHIP_NO_ERROR;

    DeviceEnergyManagementDelegate delegate_no_mfg;
    delegate_no_mfg.SetupDelegate(1);
    set_power_capability(delegate_no_mfg);
    delegate_no_mfg.PowerAdjustRequest(100, 60, AdjustmentCauseEnum::kLocalOptimization);
    unit_test_log_event_fail(true);
    TEST_ASSERT_EQUAL(static_cast<int>(Status::Failure), static_cast<int>(delegate_no_mfg.CancelPowerAdjustRequest()));
    unit_test_log_event_fail(false);
}

static void test_start_time_adjust_bad_cause(void)
{
    reset_test_harness();
    DeviceEnergyManagementDelegate delegate;
    delegate.SetupDelegate(1);
    set_forecast(delegate);

    TEST_ASSERT_EQUAL(static_cast<int>(Status::Failure),
                      static_cast<int>(delegate.StartTimeAdjustRequest(1500, AdjustmentCauseEnum::kUnknownEnumValue)));
}

static void test_pause_second_request_and_log_event_failure(void)
{
    reset_test_harness();
    DeviceEnergyManagementDelegate delegate;
    delegate.SetupDelegate(1);
    set_forecast(delegate);

    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success),
                      static_cast<int>(delegate.PauseRequest(10, AdjustmentCauseEnum::kLocalOptimization)));
    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success),
                      static_cast<int>(delegate.PauseRequest(20, AdjustmentCauseEnum::kGridOptimization)));

    DeviceEnergyManagementDelegate delegate2;
    delegate2.SetupDelegate(1);
    set_forecast(delegate2);
    unit_test_log_event_fail(true);
    TEST_ASSERT_EQUAL(static_cast<int>(Status::Failure),
                      static_cast<int>(delegate2.PauseRequest(5, AdjustmentCauseEnum::kLocalOptimization)));
    unit_test_log_event_fail(false);
}

static void test_resume_request_failures(void)
{
    reset_test_harness();
    DeviceEnergyManagementDelegate delegate;
    TestDemManufacturerDelegate manufacturer;
    delegate.SetupDelegate(1);
    set_forecast(delegate);
    delegate.SetDEMManufacturerDelegate(manufacturer);

    TEST_ASSERT_EQUAL(static_cast<int>(Status::Failure), static_cast<int>(delegate.ResumeRequest()));

    delegate.PauseRequest(10, AdjustmentCauseEnum::kLocalOptimization);
    unit_test_log_event_fail(true);
    TEST_ASSERT_EQUAL(static_cast<int>(Status::Failure), static_cast<int>(delegate.ResumeRequest()));
    unit_test_log_event_fail(false);

    delegate.PauseRequest(10, AdjustmentCauseEnum::kLocalOptimization);
    manufacturer.cancel_pause_err = CHIP_ERROR_INTERNAL;
    TEST_ASSERT_EQUAL(static_cast<int>(Status::Failure), static_cast<int>(delegate.ResumeRequest()));
}

static void test_modify_forecast_without_manufacturer(void)
{
    reset_test_harness();
    DeviceEnergyManagementDelegate delegate;
    delegate.SetupDelegate(1);
    set_forecast(delegate, 7);

    DataModel::DecodableList<Structs::SlotAdjustmentStruct::DecodableType> slots{};
    TEST_ASSERT_EQUAL(
        static_cast<int>(Status::Success),
        static_cast<int>(delegate.ModifyForecastRequest(7, slots, AdjustmentCauseEnum::kLocalOptimization)));
    TEST_ASSERT_EQUAL(static_cast<int>(ForecastUpdateReasonEnum::kLocalOptimization),
                      static_cast<int>(delegate.GetForecast().Value().forecastUpdateReason));
}

static void test_set_esa_type_reports_attribute_change(void)
{
    reset_test_harness();
    DeviceEnergyManagementDelegate delegate;
    delegate.SetupDelegate(1);

    const int before = MatterManager::ReportAttributeCallCount();
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetESAType(ESATypeEnum::kSpaceHeater));
    TEST_ASSERT_TRUE(MatterManager::ReportAttributeCallCount() > before);
}

static void test_set_opt_out_normalizes_forecast_reason(void)
{
    reset_test_harness();
    DeviceEnergyManagementDelegate delegate;
    delegate.SetupDelegate(1);

    Structs::ForecastStruct::Type forecast{};
    forecast.forecastID = 1;
    forecast.startTime = 100;
    forecast.endTime = 200;
    forecast.forecastUpdateReason = ForecastUpdateReasonEnum::kGridOptimization;
    Nullable<Structs::ForecastStruct::Type> wrapped;
    wrapped.SetNonNull(forecast);
    delegate.SetForecast(wrapped);

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetOptOutState(OptOutStateEnum::kGridOptOut));
    TEST_ASSERT_EQUAL(static_cast<int>(ForecastUpdateReasonEnum::kInternalOptimization),
                      static_cast<int>(delegate.GetForecast().Value().forecastUpdateReason));
}

void run_test_device_energy_management_delegate_impl_tests(void)
{
    RUN_TEST(test_constructor_defaults);
    RUN_TEST(test_setup_delegate_and_has_feature);
    RUN_TEST(test_setters_and_getters);
    RUN_TEST(test_power_adjust_request_local_success);
    RUN_TEST(test_power_adjust_request_grid_and_second_request);
    RUN_TEST(test_power_adjust_request_bad_cause_and_failures);
    RUN_TEST(test_cancel_power_adjust_request);
    RUN_TEST(test_start_time_adjust_request);
    RUN_TEST(test_pause_and_resume_request);
    RUN_TEST(test_pause_request_failures);
    RUN_TEST(test_modify_and_constraint_forecast_requests);
    RUN_TEST(test_cancel_request);
    RUN_TEST(test_set_opt_out_state_cancels_adjustments);
    RUN_TEST(test_normalize_forecast_bad_reason);
    RUN_TEST(test_power_adjust_get_epoch_failure);
    RUN_TEST(test_cancel_power_adjust_manufacturer_and_event_failures);
    RUN_TEST(test_start_time_adjust_bad_cause);
    RUN_TEST(test_pause_second_request_and_log_event_failure);
    RUN_TEST(test_resume_request_failures);
    RUN_TEST(test_modify_forecast_without_manufacturer);
    RUN_TEST(test_set_esa_type_reports_attribute_change);
    RUN_TEST(test_set_opt_out_normalizes_forecast_reason);
}

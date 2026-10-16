// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include "EnergyEvseDelegateImpl.h"
#include "EnergyTimeUtils.h"
#include "chip_event_logging.h"
#include "chip_system_layer.h"
#include "matterManager.h"
#include "unit_test_exports.h"

#include "app/server/Server.h"

#include "unity.h"

#include <vector>

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::EnergyEvse;
using namespace chip::Protocols::InteractionModel;
using CT::Charger::MatterManager;

namespace {

void reset_harness(void)
{
    DeviceLayer::SystemLayer().ResetForTest();
    unit_test_log_event_fail(false);
    MatterManager::GetInstance().resetForTest();
    MatterManager::ReportAttributeCallCount() = 0;
    System::SystemClockInstance().SetRealTimeMsForTest(1'700'000'000LL * 1000);
    Server::GetInstance().GetInMemoryStorage().ClearForTest();
}

void init_targets_delegate(EvseTargetsDelegate &targets)
{
    targets.Init(&Server::GetInstance().GetPersistentStorage());
}

Structs::ChargingTargetScheduleStruct::DecodableType make_all_days_target(uint16_t minutes, uint8_t soc)
{
    Structs::ChargingTargetScheduleStruct::DecodableType schedule{};
    schedule.dayOfWeekForSequence = BitMask<TargetDayOfWeekBitmap>(kAllTargetDaysMask);
    Structs::ChargingTargetStruct::DecodableType target{};
    target.targetTimeMinutesPastMidnight = minutes;
    target.targetSoC.SetValue(soc);
    schedule.chargingTargets.SetItemsForTest({target});
    return schedule;
}

Structs::ChargingTargetScheduleStruct::DecodableType make_all_days_target_energy(uint16_t minutes, int64_t energy_mwh)
{
    Structs::ChargingTargetScheduleStruct::DecodableType schedule{};
    schedule.dayOfWeekForSequence = BitMask<TargetDayOfWeekBitmap>(kAllTargetDaysMask);
    Structs::ChargingTargetStruct::DecodableType target{};
    target.targetTimeMinutesPastMidnight = minutes;
    target.addedEnergy.SetValue(energy_mwh);
    schedule.chargingTargets.SetItemsForTest({target});
    return schedule;
}

void invoke_application_callback(EnergyEvseDelegate &delegate, EVSECallbackType type)
{
    EVSECbInfo cb{};
    cb.type = type;
    cb.ChargingCurrent.maximumChargeCurrent = 12000;
    EnergyEvseDelegate::ApplicationCallbackHandler(&cb, reinterpret_cast<intptr_t>(&delegate));
}

} // namespace

static void test_target_skipped_as_past(void)
{
    TEST_ASSERT_TRUE(EnergyEvse_TargetSkippedAsPast(100, 200, false));
    TEST_ASSERT_FALSE(EnergyEvse_TargetSkippedAsPast(300, 200, false));
    TEST_ASSERT_FALSE(EnergyEvse_TargetSkippedAsPast(100, 200, true));
}

static void test_setup_delegate_and_late_setup(void)
{
    reset_harness();
    EnergyEvseDelegate delegate;
    delegate.SetupDelegate(2);
    TEST_ASSERT_EQUAL(2, delegate.GetEndpointId());

    init_targets_delegate(*delegate.GetEvseTargetsDelegate());

    static int callback_count = 0;
    delegate.HwRegisterEvseCallbackHandler([](const EVSECbInfo *, intptr_t) { ++callback_count; }, 0);

    delegate.LateSetupAfterMatter();
    TEST_ASSERT_NOT_NULL(delegate.mCallbacks.handler);
}

static void test_enable_charging_validation_and_success(void)
{
    reset_harness();
    EnergyEvseDelegate delegate;
    delegate.SetupDelegate(1);
    init_targets_delegate(*delegate.GetEvseTargetsDelegate());

    DataModel::Nullable<uint32_t> until;
    until.SetNonNull(1'800'000'000U);

    TEST_ASSERT_EQUAL(static_cast<int>(Status::ConstraintError),
                      static_cast<int>(delegate.EnableCharging(until, 1000, 500)));
    TEST_ASSERT_EQUAL(static_cast<int>(Status::ConstraintError),
                      static_cast<int>(delegate.EnableCharging(until, 7000, 5000)));
    TEST_ASSERT_EQUAL(static_cast<int>(Status::ConstraintError),
                      static_cast<int>(delegate.EnableCharging(until, 8000, 7000)));

    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success), static_cast<int>(delegate.EnableCharging(until, 7000, 16000)));
    TEST_ASSERT_EQUAL(static_cast<int>(SupplyStateEnum::kChargingEnabled), static_cast<int>(delegate.GetSupplyState()));
}

static void test_enable_discharging_unsupported(void)
{
    reset_harness();
    EnergyEvseDelegate delegate;
    DataModel::Nullable<uint32_t> until;
    until.SetNonNull(100U);
    TEST_ASSERT_EQUAL(static_cast<int>(Status::UnsupportedAttribute),
                      static_cast<int>(delegate.EnableDischarging(until, 5000)));
}

static void test_disable_and_diagnostics(void)
{
    reset_harness();
    EnergyEvseDelegate delegate;
    delegate.SetupDelegate(1);
    init_targets_delegate(*delegate.GetEvseTargetsDelegate());

    delegate.EnableCharging(DataModel::Nullable<uint32_t>(), 7000, 16000);
    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success), static_cast<int>(delegate.Disable()));
    TEST_ASSERT_EQUAL(static_cast<int>(SupplyStateEnum::kDisabled), static_cast<int>(delegate.GetSupplyState()));

    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success), static_cast<int>(delegate.StartDiagnostics()));
    TEST_ASSERT_EQUAL(static_cast<int>(SupplyStateEnum::kDisabledDiagnostics),
                      static_cast<int>(delegate.GetSupplyState()));

    DeviceLayer::SystemLayer().FireExpiredTimersForTest();
    TEST_ASSERT_EQUAL(static_cast<int>(SupplyStateEnum::kDisabled), static_cast<int>(delegate.GetSupplyState()));
}

static void test_targets_crud(void)
{
    reset_harness();
    EnergyEvseDelegate delegate;
    delegate.SetupDelegate(1);
    EvseTargetsDelegate &targets = *delegate.GetEvseTargetsDelegate();
    init_targets_delegate(targets);

    auto schedule = make_all_days_target(600, 90);
    DataModel::DecodableList<Structs::ChargingTargetScheduleStruct::DecodableType> incoming;
    incoming.SetItemsForTest({schedule});

    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success), static_cast<int>(delegate.SetTargets(incoming)));
    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success), static_cast<int>(delegate.LoadTargets()));

    DataModel::List<const Structs::ChargingTargetScheduleStruct::Type> out;
    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success), static_cast<int>(delegate.GetTargets(out)));
    TEST_ASSERT_EQUAL(1u, out.size());

    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success), static_cast<int>(delegate.ClearTargets()));
    TEST_ASSERT_EQUAL(0u, targets.GetTargets().size());
}

static void test_setters_and_session(void)
{
    reset_harness();
    EnergyEvseDelegate delegate;
    delegate.SetupDelegate(1);

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetState(StateEnum::kPluggedInDemand));
    TEST_ASSERT_EQUAL(static_cast<int>(StateEnum::kPluggedInDemand), static_cast<int>(delegate.GetState()));
    TEST_ASSERT_EQUAL(CHIP_ERROR_INVALID_ARGUMENT, delegate.SetState(StateEnum::kUnknownEnumValue));

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetSupplyState(SupplyStateEnum::kChargingEnabled));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetFaultState(FaultStateEnum::kNoError));

    DataModel::Nullable<uint32_t> until;
    until.SetNonNull(99U);
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetChargingEnabledUntil(until));
    TEST_ASSERT_FALSE(delegate.GetChargingEnabledUntil().IsNull());

    delegate.mSession.SetEndpointId(1);
    delegate.mSession.StartSession(1000);
    TEST_ASSERT_FALSE(delegate.GetSessionID().IsNull());
    delegate.mSession.UpdateEnergyCharged(5000);
    TEST_ASSERT_EQUAL(4000, delegate.GetSessionEnergyCharged().Value());
    delegate.mSession.StopSession(5000);
}

static void test_compute_max_charge_current_limit(void)
{
    reset_harness();
    EnergyEvseDelegate delegate;
    delegate.SetupDelegate(1);

    static int limit_updates = 0;
    delegate.HwRegisterEvseCallbackHandler(
        [](const EVSECbInfo *cb, intptr_t) {
            if (cb->type == EVSECallbackType::ChargeCurrentChanged) {
                ++limit_updates;
            }
        },
        0);

    delegate.HwSetMaxHardwareCurrentLimit(32000);
    delegate.EnableCharging(DataModel::Nullable<uint32_t>(), 7000, 16000);

    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success), static_cast<int>(delegate.ComputeMaxChargeCurrentLimit()));
    TEST_ASSERT_EQUAL(16000, delegate.GetMaximumChargeCurrent());
    TEST_ASSERT_TRUE(limit_updates > 0);
}

static void test_find_next_target_and_schedule(void)
{
    reset_harness();
    EnergyEvseDelegate delegate;
    delegate.SetupDelegate(1);
    init_targets_delegate(*delegate.GetEvseTargetsDelegate());

    auto schedule = make_all_days_target(720, 100);
    DataModel::DecodableList<Structs::ChargingTargetScheduleStruct::DecodableType> incoming;
    incoming.SetItemsForTest({schedule});
    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success), static_cast<int>(delegate.SetTargets(incoming)));

    BitMask<TargetDayOfWeekBitmap> dayMap(kAllTargetDaysMask);
    uint16_t targetMinutes = 0;
    DataModel::Nullable<Percent> targetSoC;
    DataModel::Nullable<int64_t> addedEnergy;
    targetSoC.SetNull();
    addedEnergy.SetNull();

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR,
                      delegate.FindNextTarget(dayMap, 600, targetMinutes, targetSoC, addedEnergy, false));
    TEST_ASSERT_EQUAL(720u, targetMinutes);
    TEST_ASSERT_FALSE(targetSoC.IsNull());

    delegate.SetState(StateEnum::kPluggedInDemand);
    delegate.SetSupplyState(SupplyStateEnum::kChargingEnabled);
    MatterManager::GetInstance().isConnected = true;

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.ComputeChargingSchedule());
    TEST_ASSERT_FALSE(delegate.GetNextChargeTargetTime().IsNull());
}

static void test_fault_and_callback_paths(void)
{
    reset_harness();
    EnergyEvseDelegate delegate;
    delegate.SetupDelegate(1);
    init_targets_delegate(*delegate.GetEvseTargetsDelegate());
    delegate.SetSupplyState(SupplyStateEnum::kDisabledDiagnostics);

    TEST_ASSERT_EQUAL(static_cast<int>(Status::Failure), static_cast<int>(delegate.CheckFaultOrDiagnostic()));

    delegate.SetFaultState(FaultStateEnum::kNoError);
    delegate.SetSupplyState(SupplyStateEnum::kDisabled);

    static int state_changes = 0;
    delegate.HwRegisterEvseCallbackHandler(
        [](const EVSECbInfo *cb, intptr_t) {
            if (cb->type == EVSECallbackType::StateChanged) {
                ++state_changes;
            }
        },
        0);

    delegate.SetState(StateEnum::kPluggedInDemand);
    delegate.EnableCharging(DataModel::Nullable<uint32_t>(), 7000, 16000);
    TEST_ASSERT_TRUE(state_changes > 0);

    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success),
                      static_cast<int>(delegate.HwSetFault(FaultStateEnum::kOverCurrent)));
    TEST_ASSERT_EQUAL(static_cast<int>(FaultStateEnum::kOverCurrent), static_cast<int>(delegate.GetFaultState()));

    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success), static_cast<int>(delegate.HandleFaultCleared()));
}

static void test_hw_set_max_current_too_low(void)
{
    reset_harness();
    EnergyEvseDelegate delegate;
    TEST_ASSERT_EQUAL(static_cast<int>(Status::ConstraintError),
                      static_cast<int>(delegate.HwSetMaxHardwareCurrentLimit(-1)));
}

static void test_schedule_enable_timeout_timer(void)
{
    reset_harness();
    EnergyEvseDelegate delegate;
    delegate.SetupDelegate(1);

    uint32_t chipEpoch = 0;
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, chip::app::Clusters::DeviceEnergyManagement::GetEpochTS(chipEpoch));

    DataModel::Nullable<uint32_t> until;
    until.SetNonNull(chipEpoch - 10U);
    delegate.EnableCharging(until, 7000, 16000);

    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success), static_cast<int>(delegate.ScheduleCheckOnEnabledTimeout()));
    TEST_ASSERT_EQUAL(static_cast<int>(SupplyStateEnum::kDisabled), static_cast<int>(delegate.GetSupplyState()));
}

static void test_events_and_lambdas(void)
{
    reset_harness();
    EnergyEvseDelegate delegate;
    delegate.SetupDelegate(1);

    const uint8_t uid[] = {0x01, 0x02, 0x03};
    ByteSpan uidSpan{uid, sizeof(uid)};
    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success), static_cast<int>(delegate.SendEvent_DetectedCard(uidSpan)));
    DeviceLayer::SystemLayer().FireExpiredTimersForTest();

    TEST_ASSERT_EQUAL(static_cast<int>(Status::Failure), static_cast<int>(delegate.SendEVConnectedEvent()));

    delegate.mSession.SetEndpointId(1);
    delegate.mSession.StartSession(0);
    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success), static_cast<int>(delegate.SendEVConnectedEvent()));
    DeviceLayer::SystemLayer().FireExpiredTimersForTest();

    delegate.SetState(StateEnum::kPluggedInCharging);
    delegate.SetMaximumChargeCurrent(16000);
    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success), static_cast<int>(delegate.SendEnergyTransferStartedEvent()));
    DeviceLayer::SystemLayer().FireExpiredTimersForTest();

    delegate.SetState(StateEnum::kPluggedInDischarging);
    delegate.SetMaximumDischargeCurrent(8000);
    delegate.mSession.UpdateEnergyDischarged(5000);
    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success), static_cast<int>(delegate.SendEnergyTransferStartedEvent()));
    TEST_ASSERT_EQUAL(
        static_cast<int>(Status::Success),
        static_cast<int>(delegate.SendEnergyTransferStoppedEvent(EnergyTransferStoppedReasonEnum::kEVStopped)));
    DeviceLayer::SystemLayer().FireExpiredTimersForTest();

    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success), static_cast<int>(delegate.SendEVNotDetectedEvent()));
    DeviceLayer::SystemLayer().FireExpiredTimersForTest();

    unit_test_log_event_fail(true);
    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success), static_cast<int>(delegate.SendEvent_DetectedCard(uidSpan)));
    DeviceLayer::SystemLayer().FireExpiredTimersForTest();
    unit_test_log_event_fail(false);
}

static void test_attribute_setters_extended(void)
{
    reset_harness();
    EnergyEvseDelegate delegate;
    delegate.SetupDelegate(3);
    MatterManager::ReportAttributeCallCount() = 0;

    TEST_ASSERT_EQUAL(CHIP_ERROR_INVALID_ARGUMENT, delegate.SetMinimumChargeCurrent(-1));
    TEST_ASSERT_EQUAL(CHIP_ERROR_INVALID_ARGUMENT, delegate.SetMaximumChargeCurrent(-1));
    TEST_ASSERT_EQUAL(CHIP_ERROR_INVALID_ARGUMENT, delegate.SetMaximumDischargeCurrent(-1));
    TEST_ASSERT_EQUAL(CHIP_ERROR_INVALID_ARGUMENT, delegate.SetUserMaximumChargeCurrent(-1));
    TEST_ASSERT_EQUAL(CHIP_ERROR_INVALID_ARGUMENT,
                      delegate.SetRandomizationDelayWindow(kMaxRandomizationDelayWindow + 1));

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetMinimumChargeCurrent(6000));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetMaximumChargeCurrent(16000));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetMaximumDischargeCurrent(5000));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetUserMaximumChargeCurrent(20000));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetRandomizationDelayWindow(120));

    DataModel::Nullable<uint32_t> epoch;
    epoch.SetNonNull(200U);
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetNextChargeStartTime(epoch));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetNextChargeTargetTime(epoch));

    DataModel::Nullable<int64_t> energy;
    energy.SetNonNull(5000);
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetNextChargeRequiredEnergy(energy));

    DataModel::Nullable<Percent> soc;
    soc.SetNonNull(80);
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetNextChargeTargetSoC(soc));

    DataModel::Nullable<uint16_t> eff;
    eff.SetNonNull(92);
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetApproximateEVEfficiency(eff));

    DataModel::Nullable<uint32_t> disUntil;
    disUntil.SetNonNull(300U);
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetDischargingEnabledUntil(disUntil));

    TEST_ASSERT_EQUAL(CHIP_ERROR_INVALID_ARGUMENT, delegate.SetSupplyState(SupplyStateEnum::kUnknownEnumValue));
    TEST_ASSERT_EQUAL(CHIP_ERROR_INVALID_ARGUMENT, delegate.SetFaultState(FaultStateEnum::kUnknownEnumValue));

    TEST_ASSERT_TRUE(MatterManager::ReportAttributeCallCount() > 0);
}

static void test_fault_restore_and_hw_set_fault(void)
{
    reset_harness();
    EnergyEvseDelegate delegate;
    delegate.SetupDelegate(1);
    delegate.SetState(StateEnum::kPluggedInDemand);
    delegate.SetSupplyState(SupplyStateEnum::kChargingEnabled);

    TEST_ASSERT_EQUAL(static_cast<int>(Status::Failure),
                      static_cast<int>(delegate.HwSetFault(FaultStateEnum::kNoError)));

    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success),
                      static_cast<int>(delegate.HwSetFault(FaultStateEnum::kOverCurrent)));
    TEST_ASSERT_EQUAL(static_cast<int>(StateEnum::kFault), static_cast<int>(delegate.GetState()));
    DeviceLayer::SystemLayer().FireExpiredTimersForTest();

    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success),
                      static_cast<int>(delegate.HwSetFault(FaultStateEnum::kNoError)));
    TEST_ASSERT_EQUAL(static_cast<int>(StateEnum::kPluggedInDemand), static_cast<int>(delegate.GetState()));
    TEST_ASSERT_EQUAL(static_cast<int>(SupplyStateEnum::kChargingEnabled), static_cast<int>(delegate.GetSupplyState()));

    TEST_ASSERT_EQUAL(static_cast<int>(Status::Failure), static_cast<int>(delegate.HandleFaultCleared()));
}

static void test_schedule_with_added_energy(void)
{
    reset_harness();
    EnergyEvseDelegate delegate;
    delegate.SetupDelegate(1);
    init_targets_delegate(*delegate.GetEvseTargetsDelegate());

    auto schedule = make_all_days_target_energy(900, 50'000);
    DataModel::DecodableList<Structs::ChargingTargetScheduleStruct::DecodableType> incoming;
    incoming.SetItemsForTest({schedule});
    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success), static_cast<int>(delegate.SetTargets(incoming)));

    delegate.SetState(StateEnum::kPluggedInDemand);
    delegate.SetSupplyState(SupplyStateEnum::kChargingEnabled);
    delegate.SetMaximumChargeCurrent(16000);
    MatterManager::GetInstance().isConnected = true;

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.ComputeChargingSchedule());
    TEST_ASSERT_FALSE(delegate.GetNextChargeRequiredEnergy().IsNull());
    TEST_ASSERT_FALSE(delegate.GetNextChargeStartTime().IsNull());
}

static void test_plugged_in_discharging_to_charging(void)
{
    reset_harness();
    EnergyEvseDelegate delegate;
    delegate.SetupDelegate(1);
    delegate.mSession.SetEndpointId(1);
    delegate.mSession.StartSession(0);
    delegate.HwSetMaxHardwareCurrentLimit(32000);
    delegate.SetState(StateEnum::kPluggedInDischarging);
    delegate.SetSupplyState(SupplyStateEnum::kChargingEnabled);

    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success),
                      static_cast<int>(delegate.EnableCharging(DataModel::Nullable<uint32_t>(), 7000, 16000)));
    TEST_ASSERT_EQUAL(static_cast<int>(StateEnum::kPluggedInCharging), static_cast<int>(delegate.GetState()));
    DeviceLayer::SystemLayer().FireExpiredTimersForTest();
}

static void test_disable_from_active_charging(void)
{
    reset_harness();
    EnergyEvseDelegate delegate;
    delegate.SetupDelegate(1);
    delegate.mSession.SetEndpointId(1);
    delegate.mSession.StartSession(0);
    delegate.SetState(StateEnum::kPluggedInCharging);
    delegate.SetSupplyState(SupplyStateEnum::kChargingEnabled);

    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success), static_cast<int>(delegate.Disable()));
    TEST_ASSERT_EQUAL(static_cast<int>(SupplyStateEnum::kDisabled), static_cast<int>(delegate.GetSupplyState()));
    DeviceLayer::SystemLayer().FireExpiredTimersForTest();
}

static void test_compute_schedule_not_connected(void)
{
    reset_harness();
    EnergyEvseDelegate delegate;
    delegate.SetupDelegate(1);
    MatterManager::GetInstance().isConnected = false;
    delegate.SetState(StateEnum::kPluggedInDemand);
    delegate.SetSupplyState(SupplyStateEnum::kChargingEnabled);
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.ComputeChargingSchedule());
}

static void test_enable_timeout_future_timer(void)
{
    reset_harness();
    EnergyEvseDelegate delegate;
    delegate.SetupDelegate(1);

    uint32_t chipEpoch = 0;
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, chip::app::Clusters::DeviceEnergyManagement::GetEpochTS(chipEpoch));

    DataModel::Nullable<uint32_t> until;
    until.SetNonNull(chipEpoch + 120U);
    delegate.EnableCharging(until, 7000, 16000);

    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success), static_cast<int>(delegate.ScheduleCheckOnEnabledTimeout()));
    TEST_ASSERT_EQUAL(static_cast<int>(SupplyStateEnum::kChargingEnabled), static_cast<int>(delegate.GetSupplyState()));
    DeviceLayer::SystemLayer().FireExpiredTimersForTest();
    TEST_ASSERT_EQUAL(static_cast<int>(SupplyStateEnum::kChargingEnabled), static_cast<int>(delegate.GetSupplyState()));
}

static void test_application_internal_callbacks(void)
{
    reset_harness();
    EnergyEvseDelegate delegate;
    delegate.SetupDelegate(1);
    init_targets_delegate(*delegate.GetEvseTargetsDelegate());
    MatterManager::GetInstance().isConnected = true;

    auto schedule = make_all_days_target(800, 100);
    DataModel::DecodableList<Structs::ChargingTargetScheduleStruct::DecodableType> incoming;
    incoming.SetItemsForTest({schedule});
    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success), static_cast<int>(delegate.SetTargets(incoming)));

    delegate.SetState(StateEnum::kPluggedInDemand);
    delegate.SetSupplyState(SupplyStateEnum::kChargingEnabled);

    invoke_application_callback(delegate, EVSECallbackType::StateChanged);
    invoke_application_callback(delegate, EVSECallbackType::ChargeCurrentChanged);
    invoke_application_callback(delegate, EVSECallbackType::ChargingPreferencesChanged);
    invoke_application_callback(delegate, EVSECallbackType::EnergyMeterReadingRequested);
    invoke_application_callback(delegate, static_cast<EVSECallbackType>(99));
    DeviceLayer::SystemLayer().FireExpiredTimersForTest();
}

static void test_start_diagnostics_requires_disabled(void)
{
    reset_harness();
    EnergyEvseDelegate delegate;
    delegate.SetupDelegate(1);
    delegate.EnableCharging(DataModel::Nullable<uint32_t>(), 7000, 16000);
    TEST_ASSERT_EQUAL(static_cast<int>(Status::Failure), static_cast<int>(delegate.StartDiagnostics()));
}

static void test_hw_register_duplicate_callback(void)
{
    reset_harness();
    EnergyEvseDelegate delegate;
    delegate.SetupDelegate(1);
    delegate.LateSetupAfterMatter();
    TEST_ASSERT_EQUAL(static_cast<int>(Status::Failure),
                      static_cast<int>(delegate.HwRegisterEvseCallbackHandler([](const EVSECbInfo *, intptr_t) {}, 0)));
}

static void test_find_next_target_picks_earlier(void)
{
    reset_harness();
    EnergyEvseDelegate delegate;
    delegate.SetupDelegate(1);
    init_targets_delegate(*delegate.GetEvseTargetsDelegate());

    Structs::ChargingTargetScheduleStruct::DecodableType schedule{};
    schedule.dayOfWeekForSequence = BitMask<TargetDayOfWeekBitmap>(kAllTargetDaysMask);
    Structs::ChargingTargetStruct::DecodableType early{};
    early.targetTimeMinutesPastMidnight = 700;
    early.targetSoC.SetValue(80);
    Structs::ChargingTargetStruct::DecodableType late{};
    late.targetTimeMinutesPastMidnight = 900;
    late.targetSoC.SetValue(100);
    schedule.chargingTargets.SetItemsForTest({late, early});

    DataModel::DecodableList<Structs::ChargingTargetScheduleStruct::DecodableType> incoming;
    incoming.SetItemsForTest({schedule});
    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success), static_cast<int>(delegate.SetTargets(incoming)));

    BitMask<TargetDayOfWeekBitmap> dayMap(kAllTargetDaysMask);
    uint16_t targetMinutes = 0;
    DataModel::Nullable<Percent> targetSoC;
    DataModel::Nullable<int64_t> addedEnergy;
    targetSoC.SetNull();
    addedEnergy.SetNull();

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR,
                      delegate.FindNextTarget(dayMap, 600, targetMinutes, targetSoC, addedEnergy, false));
    TEST_ASSERT_EQUAL(700u, targetMinutes);
    TEST_ASSERT_EQUAL(80, targetSoC.Value());
}

void run_test_energy_evse_delegate_impl_tests(void)
{
    RUN_TEST(test_target_skipped_as_past);
    RUN_TEST(test_setup_delegate_and_late_setup);
    RUN_TEST(test_enable_charging_validation_and_success);
    RUN_TEST(test_enable_discharging_unsupported);
    RUN_TEST(test_disable_and_diagnostics);
    RUN_TEST(test_targets_crud);
    RUN_TEST(test_setters_and_session);
    RUN_TEST(test_compute_max_charge_current_limit);
    RUN_TEST(test_find_next_target_and_schedule);
    RUN_TEST(test_fault_and_callback_paths);
    RUN_TEST(test_hw_set_max_current_too_low);
    RUN_TEST(test_schedule_enable_timeout_timer);
    RUN_TEST(test_events_and_lambdas);
    RUN_TEST(test_attribute_setters_extended);
    RUN_TEST(test_fault_restore_and_hw_set_fault);
    RUN_TEST(test_schedule_with_added_energy);
    RUN_TEST(test_plugged_in_discharging_to_charging);
    RUN_TEST(test_disable_from_active_charging);
    RUN_TEST(test_compute_schedule_not_connected);
    RUN_TEST(test_enable_timeout_future_timer);
    RUN_TEST(test_application_internal_callbacks);
    RUN_TEST(test_start_diagnostics_requires_disabled);
    RUN_TEST(test_hw_register_duplicate_callback);
    RUN_TEST(test_find_next_target_picks_earlier);
}

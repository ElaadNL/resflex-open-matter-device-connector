// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include "matterManager_host.h"
#include "app/clusters/electrical-energy-measurement-server/electrical-energy-measurement-server.h"
#include "app/server/Server.h"
#include "chargerManager.h"
#include "chip_system_layer.h"
#include "helpers.h"
#include "nvs.h"

#include "unity.h"

#include <ctime>

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::EnergyEvse;
using namespace chip::DeviceLayer;
using namespace CT::Charger;

namespace {

void reset_harness(void)
{
    MatterManager::GetInstance().resetForTest();
    MatterManager::GetInstance().isConnected = true;
    MatterManager::ReportAttributeCallCount() = 0;
    esp_matter::MatterStartReturnForTest() = ESP_OK;
    System::SystemClockInstance().SetRealTimeMsForTest(1'700'000'000LL * 1000);
}

ChipDeviceEvent make_event(uint16_t type)
{
    ChipDeviceEvent event{};
    event.Type = type;
    return event;
}

ChipDeviceEvent make_sta_got_ip_event(void)
{
    ChipDeviceEvent event{};
    event.Type = DeviceEventType::kESPSystemEvent;
    event.Platform.ESPSystemEvent.Base = IP_EVENT;
    event.Platform.ESPSystemEvent.Id = IP_EVENT_STA_GOT_IP;
    return event;
}

} // namespace

static void test_constructor_defaults(void)
{
    reset_harness();
    auto &mgr = MatterManager::GetInstance();
    mgr.isConnected = false;
    TEST_ASSERT_FALSE(mgr.isConnected);
    TEST_ASSERT_NULL(mgr.device_node);
}

static void test_get_charging_enabled_follows_supply_state(void)
{
    reset_harness();
    auto &mgr = MatterManager::GetInstance();
    mgr.EE_dg.SetupDelegate(1);
    mgr.EE_dg.HwSetMaxHardwareCurrentLimit(32000);
    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success),
                      static_cast<int>(mgr.EE_dg.EnableCharging(DataModel::Nullable<uint32_t>(), 7000, 16000)));
    TEST_ASSERT_TRUE(mgr.GetChargingEnabled());
    TEST_ASSERT_EQUAL(static_cast<int>(Status::Success), static_cast<int>(mgr.EE_dg.Disable()));
    TEST_ASSERT_FALSE(mgr.GetChargingEnabled());
}

static void test_is_charging_allowed_by_targets_override(void)
{
    reset_harness();
    auto &mgr = MatterManager::GetInstance();
    mgr.SetTargetsAllowForTest(true);
    TEST_ASSERT_TRUE(mgr.IsChargingAllowedByTargets());
    mgr.SetTargetsAllowForTest(false);
    TEST_ASSERT_FALSE(mgr.IsChargingAllowedByTargets());
}

static void test_is_charging_allowed_schedule_window(void)
{
    reset_harness();
    auto &mgr = MatterManager::GetInstance();

    const time_t now = time(nullptr);

    DataModel::Nullable<uint32_t> start;
    DataModel::Nullable<uint32_t> target;
    start.SetNonNull(static_cast<uint32_t>(now + 3600 - chip::kChipEpochSecondsSinceUnixEpoch));
    target.SetNonNull(static_cast<uint32_t>(now + 7200 - chip::kChipEpochSecondsSinceUnixEpoch));
    mgr.EE_dg.SetNextChargeStartTime(start);
    mgr.EE_dg.SetNextChargeTargetTime(target);

    TEST_ASSERT_FALSE(mgr.IsChargingAllowedByTargets());

    start.SetNonNull(static_cast<uint32_t>(now - 100 - chip::kChipEpochSecondsSinceUnixEpoch));
    target.SetNonNull(static_cast<uint32_t>(now + 100 - chip::kChipEpochSecondsSinceUnixEpoch));
    mgr.EE_dg.SetNextChargeStartTime(start);
    mgr.EE_dg.SetNextChargeTargetTime(target);
    TEST_ASSERT_TRUE(mgr.IsChargingAllowedByTargets());

    start.SetNonNull(static_cast<uint32_t>(now - 200 - chip::kChipEpochSecondsSinceUnixEpoch));
    target.SetNonNull(static_cast<uint32_t>(now - 10 - chip::kChipEpochSecondsSinceUnixEpoch));
    mgr.EE_dg.SetNextChargeStartTime(start);
    mgr.EE_dg.SetNextChargeTargetTime(target);
    TEST_ASSERT_FALSE(mgr.IsChargingAllowedByTargets());
}

static void test_report_attribute_when_disconnected(void)
{
    reset_harness();
    auto &mgr = MatterManager::GetInstance();
    mgr.isConnected = false;
    MatterManager::ReportAttributeChangeToMatter(1, 2, 3);
    DeviceLayer::SystemLayer().FireExpiredTimersForTest();
    TEST_ASSERT_EQUAL(0, MatterManager::ReportAttributeCallCount());
}

static void test_report_attribute_when_connected(void)
{
    reset_harness();
    auto &mgr = MatterManager::GetInstance();
    mgr.isConnected = true;
    MatterManager::ReportAttributeChangeToMatter(1, 2, 3);
    DeviceLayer::SystemLayer().FireExpiredTimersForTest();
    TEST_ASSERT_EQUAL(1, MatterManager::ReportAttributeCallCount());
}

static void test_handle_matter_events_fabric_and_ip(void)
{
    reset_harness();
    auto &mgr = MatterManager::GetInstance();

    ChipDeviceEvent ipEvent = make_event(DeviceEventType::kInterfaceIpAddressChanged);
    mgr.HandleMatterEventCb(&ipEvent);
    TEST_ASSERT_FALSE(mgr.isConnected);

    Server::GetInstance().GetFabricTable().SetFabricCountForTest(1);
    ChipDeviceEvent commissioned = make_event(DeviceEventType::kCommissioningComplete);
    mgr.HandleMatterEventCb(&commissioned);
    TEST_ASSERT_TRUE(mgr.isConnected);

    ChipDeviceEvent fabricRemoved = make_event(DeviceEventType::kFabricRemoved);
    Server::GetInstance().GetFabricTable().SetFabricCountForTest(0);
    mgr.HandleMatterEventCb(&fabricRemoved);
    TEST_ASSERT_FALSE(mgr.isConnected);
}

static void test_commissioning_window_closed_reopens_without_fabric(void)
{
    reset_harness();
    auto &mgr = MatterManager::GetInstance();
    Server::GetInstance().GetFabricTable().SetFabricCountForTest(0);
    Server::GetInstance().GetCommissioningWindowManager().ResetForTest();

    ChipDeviceEvent closed = make_event(DeviceEventType::kCommissioningWindowClosed);
    mgr.HandleMatterEventCb(&closed);
    TEST_ASSERT_TRUE(Server::GetInstance().GetCommissioningWindowManager().IsCommissioningWindowOpen());
}

static void test_wrapper_event_cb(void)
{
    reset_harness();
    auto &mgr = MatterManager::GetInstance();
    ChipDeviceEvent event = make_event(DeviceEventType::kCommissioningSessionStarted);
    MatterManager::Wrapper_MatterEventCb(&event, reinterpret_cast<intptr_t>(&mgr));
    MatterManager::Wrapper_MatterEventCb(&event, 0);
}

static void test_handle_attribute_update_paths(void)
{
    reset_harness();
    auto &mgr = MatterManager::GetInstance();
    mgr.EE_dg.SetupDelegate(5);

    esp_matter_attr_val_t val{};
    val.val.i64 = 12000;
    mgr.HandleMatterAttributeUpdate(99, EnergyEvse::Id, EnergyEvse::Attributes::UserMaximumChargeCurrent::Id, &val);

    val.val.u32 = 500;
    mgr.HandleMatterAttributeUpdate(mgr.EE_dg.GetEndpointId(), EnergyEvse::Id,
                                    EnergyEvse::Attributes::ChargingEnabledUntil::Id, &val);

    val.val.p = nullptr;
    mgr.HandleMatterAttributeUpdate(mgr.EE_dg.GetEndpointId(), EnergyEvse::Id,
                                    EnergyEvse::Attributes::ChargingEnabledUntil::Id, &val);
}

static void test_update_state_charger_status_mapping(void)
{
    reset_harness();
    auto &mgr = MatterManager::GetInstance();
    mgr.EE_dg.SetupDelegate(1);
    mgr.EE_dg.mSession.SetEndpointId(1);

    auto &charger = ChargerManager::Controller();
    charger.resetForTest();

    charger.status = ChargerStatus_t::AVAILABLE;
    mgr.UpdateState();
    TEST_ASSERT_EQUAL(static_cast<int>(StateEnum::kNotPluggedIn), static_cast<int>(mgr.EE_dg.GetState()));

    charger.status = ChargerStatus_t::PREPARING;
    mgr.EE_dg.mSession.StartSession(0);
    mgr.UpdateState();
    TEST_ASSERT_EQUAL(static_cast<int>(StateEnum::kPluggedInNoDemand), static_cast<int>(mgr.EE_dg.GetState()));

    charger.status = ChargerStatus_t::CHARGING;
    mgr.EE_dg.mSession.StartSession(0);
    mgr.UpdateState();
    TEST_ASSERT_EQUAL(static_cast<int>(StateEnum::kPluggedInCharging), static_cast<int>(mgr.EE_dg.GetState()));

    charger.status = ChargerStatus_t::FINISHING;
    mgr.EE_dg.mSession.StartSession(0);
    mgr.UpdateState();
    TEST_ASSERT_EQUAL(static_cast<int>(StateEnum::kSessionEnding), static_cast<int>(mgr.EE_dg.GetState()));

    charger.status = ChargerStatus_t::FAULTED;
    mgr.UpdateState();
    TEST_ASSERT_EQUAL(static_cast<int>(StateEnum::kFault), static_cast<int>(mgr.EE_dg.GetState()));
}

static void test_session_helpers(void)
{
    reset_harness();
    auto &mgr = MatterManager::GetInstance();
    mgr.EE_dg.SetupDelegate(1);
    mgr.EE_dg.mSession.SetEndpointId(1);

    mgr.StartSession(1000);
    TEST_ASSERT_FALSE(mgr.EE_dg.GetSessionID().IsNull());

    mgr.UpdateSession(5000);
    TEST_ASSERT_EQUAL(4000, mgr.EE_dg.GetSessionEnergyCharged().Value());

    mgr.StopSession(5000);
}

static void test_update_fault_state(void)
{
    reset_harness();
    auto &mgr = MatterManager::GetInstance();
    mgr.EE_dg.SetupDelegate(1);
    mgr.UpdateFaultState(static_cast<uint8_t>(FaultStateEnum::kOverCurrent));
    TEST_ASSERT_EQUAL(static_cast<int>(FaultStateEnum::kOverCurrent), static_cast<int>(mgr.EE_dg.GetFaultState()));
}

static void test_send_readings_and_power_init(void)
{
    reset_harness();
    auto &mgr = MatterManager::GetInstance();
    mgr.EPM_dg.SetupDelegate(2);

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, mgr.InitializePowerMeasurementCluster());
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, mgr.InitializePowerSourceCluster());
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, mgr.SendReadings(1000, 230000, 5000));
    TEST_ASSERT_EQUAL(1000, mgr.EPM_dg.GetActivePower().Value());
}

static void test_send_cumulative_energy_reading(void)
{
    reset_harness();
    auto &mgr = MatterManager::GetInstance();
    mgr.EPM_dg.SetupDelegate(3);
    ElectricalEnergyMeasurement::MeasurementStore()[3] = ElectricalEnergyMeasurement::MeasurementData{};

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, mgr.SendCumulativeEnergyReading(100, 50));
    DeviceLayer::SystemLayer().FireExpiredTimersForTest();
    TEST_ASSERT_TRUE(ElectricalEnergyMeasurement::MeasurementStore()[3].cumulativeImported.HasValue());
    TEST_ASSERT_EQUAL(100, ElectricalEnergyMeasurement::MeasurementStore()[3].cumulativeImported.Value().energy);
}

static void test_initialize_device_delegates_and_stack(void)
{
    reset_harness();
    auto &mgr = MatterManager::GetInstance();
    mgr.InitializeDeviceDelegates();
    mgr.InitializeMatterDeviceNode();
    TEST_ASSERT_NOT_NULL(mgr.device_node);
    TEST_ASSERT_TRUE(mgr.EE_dg.GetEndpointId() > 0);

    mgr.StartMatterStack();
}

static void test_restore_time_nvs_drift_and_first_boot(void)
{
    reset_harness();
    unit_test_nvs_reset();

    auto &mgr = MatterManager::GetInstance();
    mgr.Init();

    nvs_handle_t handle = 0;
    TEST_ASSERT_EQUAL(ESP_OK, nvs_open("storage", NVS_READWRITE, &handle));
    const time_t now = time(nullptr);
    TEST_ASSERT_EQUAL(ESP_OK, nvs_set_i64(handle, "sync_sec", static_cast<int64_t>(now - 120)));
    nvs_commit(handle);
    nvs_close(handle);

    mgr.Init();
}

static void test_power_source_delegate_setup(void)
{
    reset_harness();
    auto &mgr = MatterManager::GetInstance();
    mgr.PS_dg.SetupDelegate(4);
    mgr.PS_dg.LateSetupAfterMatter();
    TEST_ASSERT_EQUAL(4, mgr.PS_dg.mEndpointId);
}

void run_test_matter_manager_tests(void)
{
    RUN_TEST(test_constructor_defaults);
    RUN_TEST(test_get_charging_enabled_follows_supply_state);
    RUN_TEST(test_is_charging_allowed_by_targets_override);
    RUN_TEST(test_is_charging_allowed_schedule_window);
    RUN_TEST(test_report_attribute_when_disconnected);
    RUN_TEST(test_report_attribute_when_connected);
    RUN_TEST(test_handle_matter_events_fabric_and_ip);
    RUN_TEST(test_commissioning_window_closed_reopens_without_fabric);
    RUN_TEST(test_wrapper_event_cb);
    RUN_TEST(test_handle_attribute_update_paths);
    RUN_TEST(test_update_state_charger_status_mapping);
    RUN_TEST(test_session_helpers);
    RUN_TEST(test_update_fault_state);
    RUN_TEST(test_send_readings_and_power_init);
    RUN_TEST(test_send_cumulative_energy_reading);
    RUN_TEST(test_initialize_device_delegates_and_stack);
    RUN_TEST(test_restore_time_nvs_drift_and_first_boot);
    RUN_TEST(test_power_source_delegate_setup);
}

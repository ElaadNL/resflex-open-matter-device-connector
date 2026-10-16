// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#ifdef UNIT_TEST
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "helpers.h"
#include "get_readable_time.h"
#include "common_macros.h"
#include "chip_support.h"
#include "chip_system_layer.h"
#include "device.h"
#include "esp_wifi.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_matter.h"
#include "esp_sntp.h"
#include "nvs.h"
#include "chargerManager.h"
#include "EnergyTimeUtils.h"
#include "EnergyEvseDelegateImpl.h"
#include "matterManager_host.h"
#include "app/clusters/electrical-energy-measurement-server/electrical-energy-measurement-server.h"
#include "app/server/Server.h"

#include <cmath>
#include <string>

#undef CHIP_ERROR_FORMAT
#define CHIP_ERROR_FORMAT "%d"
#define CHIP_ERR_FMT(e) static_cast<int>((e).AsInteger())
#else
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <esp_wifi.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_matter.h"
#include "esp_sntp.h"
#include "nvs_flash.h"

#if CONFIG_ENABLE_SNTP_TIME_SYNC
#include <TimeSync.h>
#endif

#if CONFIG_ENABLE_MATTER_AUTHENTICATED_TIME_SYNC
#include <app/clusters/time-synchronization-server/DefaultTimeSyncDelegate.h>
#endif

#include <device.h>
#include <protocols/Protocols.h>

#include <common_macros.h>

#include "helpers.h"
#include "get_readable_time.h"

#include "chargerManager.h"

#include <EnergyTimeUtils.h>

#include <app/clusters/electrical-energy-measurement-server/electrical-energy-measurement-server.h>

#include <app-common/zap-generated/attributes/Accessors.h>

#include "EnergyEvseDelegateImpl.h"

#include "matterManager.h"
#endif

#if CONFIG_ENABLE_SNTP_TIME_SYNC
#ifndef CONFIG_NTP_SERVER_PRIMARY
#define CONFIG_NTP_SERVER_PRIMARY "pool.ntp.org"
#endif
#ifndef CONFIG_NTP_SERVER_SECONDARY
#define CONFIG_NTP_SERVER_SECONDARY ""
#endif
#ifndef CONFIG_NTP_SERVER_TERTIARY
#define CONFIG_NTP_SERVER_TERTIARY ""
#endif
#ifndef CONFIG_NTP_SYNC_INTERVAL_DAYS
#define CONFIG_NTP_SYNC_INTERVAL_DAYS 1
#endif
#endif

using namespace chip;
using namespace chip::app;
using namespace chip::app::DataModel;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::EnergyEvse;
using namespace chip::app::Clusters::EnergyEvse::Attributes;
using namespace chip::app::Clusters::EnergyEvseMode;
using namespace chip::app::Clusters::DeviceEnergyManagement;
using namespace chip::app::Clusters::PowerSource;

using namespace chip::app::Clusters::ElectricalEnergyMeasurement;
using namespace chip::app::Clusters::ElectricalPowerMeasurement;

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::cluster;
using namespace esp_matter::endpoint;

using chip::Protocols::InteractionModel::Status;

#ifdef UNIT_TEST
namespace {
int g_report_count = 0;
bool unit_test_targets_allow_override = false;
bool unit_test_targets_allow_value = true;
} // namespace

void MatterReportingAttributeChangeCallback(chip::EndpointId endpoint, chip::ClusterId clusterId,
                                            chip::AttributeId attributeId)
{
    (void)endpoint;
    (void)clusterId;
    (void)attributeId;
    ++g_report_count;
}
#endif

namespace CT {
namespace Charger {

using chip::DeviceLayer::ChipDeviceEvent;

/** Alias for ESP-Matter callback private-data pointer (underlying type is still void *, per Espressif API). */
using EspMatterCallbackOpaquePtr = void *;

namespace {

constexpr int kCommissioningWindowTimeoutSeconds = 300;

unsigned GetActiveFabricCount()
{
    return chip::Server::GetInstance().GetFabricTable().FabricCount();
}

void UpdateIsConnectedFromFabricTable(MatterManager *self)
{
    self->isConnected = (GetActiveFabricCount() != 0);
}

/** @return true if a fabric is present (and isConnected was set true); false if none. */
bool ApplyFabricPresenceToConnectedFlag(MatterManager *self)
{
    if (GetActiveFabricCount() != 0) {
        self->isConnected = true;
        return true;
    }
    self->isConnected = false;
    return false;
}

void TryReopenBasicCommissioningWindowIfClosed()
{
#if 1
    chip::CommissioningWindowManager &commissionMgr = chip::Server::GetInstance().GetCommissioningWindowManager();
    if (commissionMgr.IsCommissioningWindowOpen()) {
        return;
    }

    constexpr auto kTimeoutSeconds = chip::System::Clock::Seconds32(kCommissioningWindowTimeoutSeconds);
    CHIP_ERROR err = commissionMgr.OpenBasicCommissioningWindow(kTimeoutSeconds,
                                                                chip::CommissioningWindowAdvertisement::kAllSupported);
    if (err != CHIP_NO_ERROR) {
        PRINTF_DEBUG("Failed to open commissioning window, err:%" CHIP_ERROR_FORMAT, err.Format());
        return;
    }
    PRINTF_DEBUG("Re-opened commissioning window for %d seconds", kCommissioningWindowTimeoutSeconds);
#endif
}

void ConsiderSntpAfterStaGotIp()
{
#if CONFIG_ENABLE_SNTP_TIME_SYNC
    // Only use NTP destinations disclosed via Kconfig — no hardcoded fallbacks.
    // Plain SNTP is unauthenticated; prefer Matter Time Synchronization (CASE).
    const char *const kNtpServers[] = {
        CONFIG_NTP_SERVER_PRIMARY,
        CONFIG_NTP_SERVER_SECONDARY,
        CONFIG_NTP_SERVER_TERTIARY,
    };
    chip::Esp32TimeSync::Init(kNtpServers, sizeof(kNtpServers) / sizeof(kNtpServers[0]),
                              static_cast<uint16_t>(CONFIG_NTP_SYNC_INTERVAL_DAYS));
#endif
}

#if CONFIG_ENABLE_MATTER_AUTHENTICATED_TIME_SYNC
void AddMatterAuthenticatedTimeSyncCluster(node_t *node)
{
    endpoint_t *root_node_ep = endpoint::get_first(node);
    if (root_node_ep == nullptr) {
        PRINTF_DEBUG("Failed to find root node endpoint for Time Synchronization");
        return;
    }

    cluster::time_synchronization::config_t time_sync_cfg;
    static chip::app::Clusters::TimeSynchronization::DefaultTimeSyncDelegate time_sync_delegate;
    time_sync_cfg.delegate = &time_sync_delegate;
    cluster_t *time_sync_cluster =
        cluster::time_synchronization::create(root_node_ep, &time_sync_cfg, CLUSTER_FLAG_SERVER);
    if (time_sync_cluster == nullptr) {
        PRINTF_DEBUG("Failed to create Matter Time Synchronization cluster");
        return;
    }
    PRINTF_DEBUG("Matter Time Synchronization cluster enabled (CASE-authenticated SetUTCTime)");
}
#endif

} // namespace

static void RestoreAndPrintTime()
{
    nvs_handle_t my_handle;
    if (nvs_open("storage", NVS_READONLY, &my_handle) != ESP_OK)
        return;

    int64_t saved_sec = 0;
    time_t now;
    time(&now); // Get current system Unix time

    if (nvs_get_i64(my_handle, "sync_sec", &saved_sec) == ESP_OK) {
        // 1. Calculate the drift (absolute difference)
        int64_t drift = std::abs((int64_t)now - saved_sec);
        if (drift > 60) {
            // TIME IS SHIFTED: Restore from NVS
            struct timeval tv = {.tv_sec = (time_t)saved_sec, .tv_usec = 0};
            if (settimeofday(&tv, nullptr) != 0) {
                PRINTF_DEBUG("Failed to set POSIX time");
                return;
            }

            PRINTF_DEBUG("Drift detected (%llds). Restored NVS time: %lld", drift, saved_sec);
        } else {
            saved_sec = now;
            // TIME IS ACCURATE: Update NVS with current system time
            nvs_set_i64(my_handle, "sync_sec", (int64_t)saved_sec);
            nvs_commit(my_handle);
            PRINTF_DEBUG("Time is within range. Updated NVS with current time: %ld", saved_sec);
        }

        // Print the time in human way
        char nowStr[20];
        time_t display_time = time(nullptr);
        struct tm tm_buf {
        };
        localtime_r(&display_time, &tm_buf);
        strftime(nowStr, sizeof(nowStr), "%Y-%m-%d %H:%M:%S", &tm_buf);

        sntp_set_sync_status(SNTP_SYNC_STATUS_COMPLETED);

        PRINTF_DEBUG("POSIX time successfully set to: %s(%ld)", nowStr, (long)display_time);
    } else {
        // First boot or no data: Save current time to NVS
        nvs_set_i64(my_handle, "sync_sec", (int64_t)now);
        nvs_commit(my_handle);
        PRINTF_DEBUG("No NVS time found. Initialized NVS with current time.");
    }

    nvs_close(my_handle);
}

static esp_err_t app_identification_cb(identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id,
                                       uint8_t effect_variant, EspMatterCallbackOpaquePtr callbackContext)
{
    (void)callbackContext;
    PRINTF_DEBUG("Identification callback: type: %d, effect: %d", type, effect_id);
    return ESP_OK;
}

static esp_err_t app_attribute_update_cb(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
                                         uint32_t attribute_id, esp_matter_attr_val_t *val,
                                         EspMatterCallbackOpaquePtr callbackContext)
{
    esp_err_t err = ESP_OK;
    PRINTF_DEBUG("Received attribute update type: %s (0x%0X 0x%0X 0x%0X)",
                 type == PRE_UPDATE        ? "PRE UPDATE"
                     : type == POST_UPDATE ? "POST UPDATE"
                     : type == READ        ? "READ"
                                           : "WRITE",
                 endpoint_id, cluster_id, attribute_id);
    // If user want to use driver, can be called from here.
    if (type == PRE_UPDATE) {
        /* Driver update */
        MatterManager::GetInstance().HandleMatterAttributeUpdate(endpoint_id, cluster_id, attribute_id, val);
    }
    (void)callbackContext;
    return err;
}

void MatterManager::HandleMatterEventCb(const ChipDeviceEvent *event)
{
    using namespace chip::DeviceLayer;

    switch (event->Type) {
    case DeviceEventType::kInterfaceIpAddressChanged:
        PRINTF_DEBUG("Interface IP Address changed");
        UpdateIsConnectedFromFabricTable(this);
        break;

    case DeviceEventType::kCommissioningComplete:
        PRINTF_DEBUG("Commissioning complete");
        UpdateIsConnectedFromFabricTable(this);
        break;

    case DeviceEventType::kFailSafeTimerExpired:
        PRINTF_DEBUG("Commissioning failed, fail safe timer expired");
        UpdateIsConnectedFromFabricTable(this);
        break;

    case DeviceEventType::kCommissioningSessionStarted:
        PRINTF_DEBUG("Commissioning session started");
        break;

    case DeviceEventType::kCommissioningSessionStopped:
        PRINTF_DEBUG("Commissioning session stopped");
        break;

    case DeviceEventType::kCommissioningWindowOpened:
        PRINTF_DEBUG("Commissioning window opened");
#if 0
        // check Wi-Fi credentials to determine if it's first boot or not, if Wi-Fi credentials exist, it means it's
        // not first boot but the device is not commissioned, so we can log different message for those two cases.
        {
            wifi_config_t wifi_config;
            esp_err_t err = esp_wifi_get_config(WIFI_IF_STA, &wifi_config);
            if (err == ESP_OK && wifi_config.sta.ssid[0] != '\0')
            {
                PRINTF_DEBUG("Fabric is removed so we disconnect from the network and clear Wi-Fi credentials for demo purposes.");
                esp_wifi_disconnect();
                wifi_config_t empty_config = {};
                esp_wifi_set_config(WIFI_IF_STA, &empty_config);
            }
        }
#endif
        break;

    case DeviceEventType::kCommissioningWindowClosed:
        PRINTF_DEBUG("Commissioning window closed");
        if (ApplyFabricPresenceToConnectedFlag(this)) {
            break;
        }
        TryReopenBasicCommissioningWindowIfClosed();
        break;

    case DeviceEventType::kFabricRemoved:
        PRINTF_DEBUG("Fabric removed successfully");
        if (ApplyFabricPresenceToConnectedFlag(this)) {
            break;
        }
        TryReopenBasicCommissioningWindowIfClosed();
        break;

    case DeviceEventType::kFabricWillBeRemoved:
        PRINTF_DEBUG("Fabric will be removed");
        break;

    case DeviceEventType::kFabricUpdated:
        PRINTF_DEBUG("Fabric is updated");
        break;

    case DeviceEventType::kFabricCommitted:
        PRINTF_DEBUG("Fabric is committed");
        break;

    case DeviceEventType::kESPSystemEvent:
        if (event->Platform.ESPSystemEvent.Base != IP_EVENT ||
            event->Platform.ESPSystemEvent.Id != IP_EVENT_STA_GOT_IP) {
            break;
        }
        PRINTF_DEBUG("IP_EVENT_STA_GOT_IP");
        ConsiderSntpAfterStaGotIp();
        break;

    default:
        PRINTF_DEBUG("Event[0x%x] Not implmentated!", event->Type);
        break;
    }
}

MatterManager &MatterManager::GetInstance()
{
    static MatterManager instance;
    return instance;
}

MatterManager::MatterManager()
{
    isConnected = false;
    device_node = nullptr;
}

void MatterManager::Init()
{
    DEBUG_CHECKPOINT("Initializing Matter Energy EVSE ...");
    this->isConnected = false;

    InitializeDeviceDelegates();
    InitializeMatterDeviceNode();

    StartMatterStack();

    // set this later than the matter stack to take over the control from the lib.
    chip::DeviceLayer::SetDeviceInstanceInfoProvider(&DIIProvider);

    RestoreAndPrintTime();

    ChargerManager::Controller().SetMatterDelegateEnergyEvse(&EE_dg);
}

void MatterManager::InitializeDeviceDelegates()
{
    EE_dg.HwSetMaxHardwareCurrentLimit(ChargerManager::Controller().config.currentLimit_HW);
}

void MatterManager::InitializeMatterDeviceNode()
{
    /* Create a Matter node and add the mandatory Root Node device type on endpoint 0 */
    node::config_t node_config;
    // node handle can be used to add/modify other endpoints.
    this->device_node = node::create(&node_config, app_attribute_update_cb, app_identification_cb);
    ABORT_APP_ON_FAILURE(this->device_node != nullptr, PRINTF_DEBUG("Failed to create Matter node"));

#if CONFIG_ENABLE_MATTER_AUTHENTICATED_TIME_SYNC
    AddMatterAuthenticatedTimeSyncCluster(this->device_node);
#endif
    /*===============================================================================================*/
    endpoint::energy_evse::config_t ee_cfg;
#if 1
    ee_cfg.energy_evse.delegate = &EE_dg;
    ee_cfg.energy_evse_mode.delegate = &EEM_dg;
    // ee_cfg.device_energy_management.delegate = &DEM_dg;
#endif
    endpoint_t *evse_endpoint = endpoint::energy_evse::create(this->device_node, &ee_cfg, ENDPOINT_FLAG_NONE, nullptr);
    if (!evse_endpoint) {
        PRINTF_DEBUG("Matter create endpoint failed");
        return;
    }
    EE_dg.SetupDelegate(endpoint::get_id(evse_endpoint));
    // DEM_dg.SetupDelegate(endpoint::get_id(evse_endpoint));
    /*===============================================================================================*/
    endpoint::power_source_device::config_t power_source_config;
    endpoint_t *ps_endpoint =
        endpoint::power_source_device::create(this->device_node, &power_source_config, ENDPOINT_FLAG_NONE, nullptr);
    if (!ps_endpoint) {
        PRINTF_DEBUG("Matter create endpoint failed");
        return;
    }
    // power source do not have the delegate
    PS_dg.SetupDelegate(endpoint::get_id(ps_endpoint));

    /*===============================================================================================*/

    endpoint::electrical_sensor::config_t es_cfg;
    es_cfg.power_topology.delegate = &PT_dg;
    es_cfg.electrical_power_measurement.delegate = &EPM_dg;

    endpoint_t *es_endpoint =
        endpoint::electrical_sensor::create(this->device_node, &es_cfg, ENDPOINT_FLAG_NONE, nullptr);
    if (!es_endpoint) {
        PRINTF_DEBUG("Matter create endpoint failed");
        return;
    }

    // endpoint::electrical_sensor::add(ps_endpoint, &es_cfg);

    PT_dg.SetupDelegate(endpoint::get_id(es_endpoint));
    // Power topology endpoints list is pointing to the actual energy sensor devivce like EVSE, so we set the available
    // endpoint to evse endpoint id here
    PT_dg.SetAvailableEndpointIds(endpoint::get_id(evse_endpoint), 0);

    EPM_dg.SetupDelegate(endpoint::get_id(es_endpoint));
    /*===============================================================================================*/
    endpoint::device_energy_management::config_t dem_cfg;
    dem_cfg.device_energy_management.delegate = &DEM_dg;

    endpoint_t *dem_endpoint =
        endpoint::device_energy_management::create(this->device_node, &dem_cfg, ENDPOINT_FLAG_NONE, nullptr);
    if (!dem_endpoint) {
        PRINTF_DEBUG("Matter create endpoint failed");
        return;
    }

    DEM_dg.SetupDelegate(endpoint::get_id(dem_endpoint));
    /*===============================================================================================*/

    DEBUG_CHECKPOINT("Matter EVSE Device created!");
}

void MatterManager::StartMatterStack()
{
    /* Matter start */
    esp_err_t err = ESP_OK;
    DEBUG_CHECKPOINT("Start Matter SDK ...");
    err = esp_matter::start(Wrapper_MatterEventCb, reinterpret_cast<intptr_t>(this));
    ABORT_APP_ON_FAILURE(err == ESP_OK, PRINTF_DEBUG("Failed to start Matter, err:%d", err));
    if (err != ESP_OK) {
        PRINTF_DEBUG("Matter start failed: %d", err);
    }

    EE_dg.LateSetupAfterMatter();
    PS_dg.LateSetupAfterMatter();
    PT_dg.LateSetupAfterMatter();

#if 0
    chip::CommissioningWindowManager & commissionMgr = chip::Server::GetInstance().GetCommissioningWindowManager();
    commissionMgr.OverrideMinCommissioningTimeout(System::Clock::Seconds32(10));
    PRINTF_DEBUG("Commissioning window will close after 10 seconds for demo purposes.");
#endif

    DEBUG_CHECKPOINT("Start Matter SDK ... Done");
}

void MatterManager::HandleMatterAttributeUpdate(uint16_t endpoint_id, uint32_t cluster_id, uint32_t attribute_id,
                                                esp_matter_attr_val_t *val)
{
    if (endpoint_id != EE_dg.GetEndpointId() || cluster_id != Clusters::EnergyEvse::Id) {
        return;
    }
    if (attribute_id == EnergyEvse::Attributes::UserMaximumChargeCurrent::Id) {
        PRINTF_DEBUG("EnergyEvse::Attributes::UserMaximumChargeCurrent %llu", val->val.i64);
        return;
    }
    if (attribute_id != EnergyEvse::Attributes::ChargingEnabledUntil::Id) {
        return;
    }
    if (val->val.p == nullptr) {
        PRINTF_DEBUG("EnergyEvse::Attributes::ChargingEnabledUntil Forever!");
    } else {
        PRINTF_DEBUG("EnergyEvse::Attributes::ChargingEnabledUntil %llu", val->val.u32);
    }
}

void MatterManager::Wrapper_MatterEventCb(const ChipDeviceEvent *event, intptr_t arg)
{
    // Recover 'this' from arg
    MatterManager *self = reinterpret_cast<MatterManager *>(arg);
    if (self) {
        self->HandleMatterEventCb(event);
    }
}

void MatterManager::UpdateState()
{
    ChargerStatus_t status = ChargerManager::Controller().status;
    StateEnum data = StateEnum::kUnknownEnumValue;

    if (status == ChargerStatus_t::AVAILABLE) {
        data = StateEnum::kNotPluggedIn;
        EE_dg.SendEVNotDetectedEvent();
    } else if (status == ChargerStatus_t::PREPARING) {
        data = StateEnum::kPluggedInNoDemand;
        EE_dg.SendEVConnectedEvent();
    } else if (status == ChargerStatus_t::SUSPENDED_EVSE) {
        data = StateEnum::kPluggedInDemand;
    } else if (status == ChargerStatus_t::SUSPENDED_EV) {
        data = StateEnum::kPluggedInNoDemand;
    } else if (status == ChargerStatus_t::CHARGING) {
        EE_dg.SendEnergyTransferStartedEvent();
        data = StateEnum::kPluggedInCharging;
    } else if (status == ChargerStatus_t::FINISHING) {
        data = StateEnum::kSessionEnding;
        EE_dg.SendEnergyTransferStoppedEvent(EnergyTransferStoppedReasonEnum::kEVSEStopped);
    } else if (status == ChargerStatus_t::FAULTED) {
        data = StateEnum::kFault;
        EE_dg.SendEnergyTransferStoppedEvent(EnergyTransferStoppedReasonEnum::kOther);
    } else if (status == ChargerStatus_t::UNAVAILABLE) {
        data = StateEnum::kUnknownEnumValue;
    }

    EE_dg.SetState(data);
}

void MatterManager::UpdateFaultState(uint8_t faultCode)
{
    FaultStateEnum f = static_cast<FaultStateEnum>(faultCode);
    EE_dg.HwSetFault(f);
}

void MatterManager::StartSession(int64_t currentEnergy)
{
    EE_dg.mSession.StartSession(currentEnergy);
}

void MatterManager::StopSession(int64_t currentEnergy)
{
    EE_dg.mSession.StopSession(currentEnergy);
}

void MatterManager::UpdateSession(int64_t currentEnergy)
{
    EE_dg.mSession.UpdateEnergyCharged(currentEnergy);
    EE_dg.mSession.RecalculateSessionDuration();
}

bool MatterManager::IsChargingAllowedByTargets(void)
{
#ifdef UNIT_TEST
    if (unit_test_targets_allow_override) {
        return unit_test_targets_allow_value;
    }
#endif
    bool allowed = true;
    time_t now = time(nullptr);

    DataModel::Nullable<uint32_t> nextStart = EE_dg.GetNextChargeStartTime();
    DataModel::Nullable<uint32_t> targetTime = EE_dg.GetNextChargeTargetTime();

    // 1. If no schedule is defined, log and allow
    if (nextStart.IsNull() || targetTime.IsNull()) {
        PRINTF_DEBUG("No charging schedule set (Null). Defaulting to ALLOW.");
        allowed = true;
    } else {
        // convert it back before comparing the current time
        time_t tStart = static_cast<time_t>(nextStart.Value() + chip::kChipEpochSecondsSinceUnixEpoch);
        time_t tTarget = static_cast<time_t>(targetTime.Value() + chip::kChipEpochSecondsSinceUnixEpoch);

        // 2. Too early logic
        if (now < tStart) {
            PRINTF_DEBUG("Charging NOT allowed. Current time is BEFORE start time.");
            allowed = false;
        }

        // 3. Too late logic
        if (now >= tTarget) {
            PRINTF_DEBUG("Charging NOT allowed. Current time has PASSED target time.");
            allowed = false;
        }

#if 0
        char nowStr[20], startStr[20], targetStr[20];
        strftime(nowStr, sizeof(nowStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
        strftime(startStr, sizeof(startStr), "%Y-%m-%d %H:%M:%S", localtime(&tStart));
        strftime(targetStr, sizeof(targetStr), "%Y-%m-%d %H:%M:%S", localtime(&tTarget));
        PRINTF_DEBUG("Checking Schedule - Now: [%s], Start: [%s], Target: [%s]", nowStr, startStr, targetStr);
#endif
    }

    return allowed;
}

CHIP_ERROR MatterManager::InitializePowerMeasurementCluster()
{
    ReturnErrorOnFailure(EPM_dg.SetPowerMode(PowerModeEnum::kAc));

    return CHIP_NO_ERROR;
}

CHIP_ERROR MatterManager::InitializePowerSourceCluster()
{
    return CHIP_NO_ERROR;
}

CHIP_ERROR MatterManager::SendReadings(int64_t aActivePower_mW, int64_t aVoltage_mV, int64_t aActiveCurrent_mA)
{
    EPM_dg.SetActivePower(MakeNullable(aActivePower_mW));
    EPM_dg.SetVoltage(MakeNullable(aVoltage_mV));
    EPM_dg.SetActiveCurrent(MakeNullable(aActiveCurrent_mA));

    return CHIP_NO_ERROR;
}

CHIP_ERROR MatterManager::SendCumulativeEnergyReading(int64_t aCumulativeEnergyImported,
                                                      int64_t aCumulativeEnergyExported)
{
    using namespace chip::app::Clusters::ElectricalEnergyMeasurement::Structs;

    MeasurementData *data = MeasurementDataForEndpoint(EPM_dg.GetEndpointId());
    VerifyOrReturnError(data != nullptr, CHIP_ERROR_UNINITIALIZED);

    EnergyMeasurementStruct::Type energyImported;
    EnergyMeasurementStruct::Type energyExported;

    /** IMPORT */
    // Copy last endTimestamp into new startTimestamp if it exists
    energyImported.startTimestamp.ClearValue();
    energyImported.startSystime.ClearValue();
    if (data->cumulativeImported.HasValue()) {
        energyImported.startTimestamp = data->cumulativeImported.Value().endTimestamp;
        energyImported.startSystime = data->cumulativeImported.Value().endSystime;
    }

    energyImported.energy = aCumulativeEnergyImported;

    /** EXPORT */
    // Copy last endTimestamp into new startTimestamp if it exists
    energyExported.startTimestamp.ClearValue();
    energyExported.startSystime.ClearValue();
    if (data->cumulativeExported.HasValue()) {
        energyExported.startTimestamp = data->cumulativeExported.Value().endTimestamp;
        energyExported.startSystime = data->cumulativeExported.Value().endSystime;
    }

    energyExported.energy = aCumulativeEnergyExported;

    // Get current timestamp
    uint32_t currentTimestamp;
    CHIP_ERROR err = GetEpochTS(currentTimestamp);
    if (err == CHIP_NO_ERROR) {
        // use EpochTS
        energyImported.endTimestamp.SetValue(currentTimestamp);
        energyExported.endTimestamp.SetValue(currentTimestamp);
    } else {
        PRINTF_DEBUG("GetEpochTS returned error getting timestamp %" CHIP_ERROR_FORMAT, err.Format());

        // use systemTime as a fallback
#ifdef UNIT_TEST
        uint64_t nowMS = static_cast<uint64_t>(chip::Server::GetInstance().TimeSinceInit());
#else
        System::Clock::Milliseconds64 system_time_ms =
            std::chrono::duration_cast<System::Clock::Milliseconds64>(chip::Server::GetInstance().TimeSinceInit());
        uint64_t nowMS = static_cast<uint64_t>(system_time_ms.count());
#endif

        energyImported.endSystime.SetValue(nowMS);
        energyExported.endSystime.SetValue(nowMS);
    }

    EndpointId mid = EPM_dg.GetEndpointId();
#ifdef UNIT_TEST
    chip::DeviceLayer::SystemLayer().ScheduleLambda([mid, energyImported, energyExported]() {
#else
    chip::DeviceLayer::SystemLayer().ScheduleLambda([mid, &energyImported, &energyExported]() {
#endif
        // call the SDK to update attributes and generate an event
        if (!NotifyCumulativeEnergyMeasured(mid, MakeOptional(energyImported), MakeOptional(energyExported))) {
            PRINTF_DEBUG("Failed to notify Cumulative Energy reading.");
        }
    });

    return CHIP_NO_ERROR;
}

void MatterManager::ReportAttributeChangeToMatter(EndpointId endpoint, ClusterId clusterId, AttributeId attributeId)
{
    if (!MatterManager::GetInstance().isConnected) {
        PRINTF_DEBUG("MatterManager::GetInstance()\n ep-id:%d, clu:%d, attr:%d", endpoint, clusterId, attributeId);
        return;
    }

    // centralized the notification calls to matter stack
#ifdef UNIT_TEST
    MatterReportingAttributeChangeCallback(endpoint, clusterId, attributeId);
#else
    chip::DeviceLayer::SystemLayer().ScheduleLambda([endpoint, clusterId, attributeId]() {
        MatterReportingAttributeChangeCallback(endpoint, clusterId, attributeId);
    });
#endif
}

bool MatterManager::GetChargingEnabled()
{
    // Question: is charging enabled depends on SupplyState??
    return (EE_dg.GetSupplyState() == SupplyStateEnum::kChargingEnabled);
}

#ifdef UNIT_TEST
int &MatterManager::ReportAttributeCallCount()
{
    return g_report_count;
}

void MatterManager::resetForTest()
{
    isConnected = true;
    device_node = nullptr;
    g_report_count = 0;
    unit_test_targets_allow_override = false;
    unit_test_targets_allow_value = true;
    chip::Server::GetInstance().ResetForTest();
    chip::app::Clusters::ElectricalEnergyMeasurement::unit_test_measurement_reset();
    chip::app::Clusters::ElectricalEnergyMeasurement::NotifyCumulativeEnergyFailForTest() = false;
    unit_test_nvs_reset();
    DeviceLayer::SystemLayer().ResetForTest();

    EE_dg.SetFaultState(FaultStateEnum::kNoError);
    EE_dg.SetSupplyState(SupplyStateEnum::kDisabled);
    EE_dg.SetState(StateEnum::kNotPluggedIn);
}

void MatterManager::SetChargingEnabledForTest(bool enabled)
{
    if (enabled) {
        EE_dg.EnableCharging(DataModel::Nullable<uint32_t>(), 7000, 16000);
    } else {
        EE_dg.Disable();
    }
}

void MatterManager::SetTargetsAllowForTest(bool allowed)
{
    unit_test_targets_allow_override = true;
    unit_test_targets_allow_value = allowed;
}
#endif

} // namespace Charger
} // namespace CT

void PowerSourceDelegate::SetupDelegate(EndpointId id)
{
    mEndpointId = id;

    config.wired.wired_current_type = to_underlying(PowerSource::WiredCurrentTypeEnum::kAc);

    AddCustomAttributes();

    AddCustomFeatures(BitMask<Feature, uint32_t>(Feature::kWired));
}

void PowerSourceDelegate::AddCustomAttributes()
{
    using namespace esp_matter::cluster::power_source;
    esp_matter::cluster_t *cluster = esp_matter::cluster::get(mEndpointId, PowerSource::Id);

    // #define PowerSource_Des "CT Matter EVSE"
    // power_source::attribute::create_description(cluster, PowerSource_Des, strlen(PowerSource_Des));

    power_source::attribute::create_wired_nominal_voltage(cluster, 0, 0, 230'000);
    power_source::attribute::create_wired_maximum_current(cluster, 0, 6'000, 32'000);
}

void PowerSourceDelegate::AddCustomFeatures(
#ifdef UNIT_TEST
    BitMask<Feature, uint32_t> aFeature)
#else
    Feature aFeature)
#endif
{
    using namespace esp_matter::cluster::power_source;

#ifndef UNIT_TEST
    mFeature.Set(aFeature);
#else
    mFeature = aFeature;
#endif

    esp_matter::cluster_t *cluster = esp_matter::cluster::get(mEndpointId, PowerSource::Id);

    if (mFeature.Has(Feature::kWired)) {
        feature::wired::add(cluster, &(config.wired));
    }
    if (mFeature.Has(Feature::kBattery)) {
        feature::battery::add(cluster, &(config.battery));
    }
    if (mFeature.Has(Feature::kRechargeable)) {
        feature::rechargeable::add(cluster, &(config.rechargeable));
    }
    if (mFeature.Has(Feature::kReplaceable)) {
        feature::replaceable::add(cluster, &(config.replaceable));
    }
}

void PowerSourceDelegate::LateSetupAfterMatter()
{
    PRINTF_DEBUG();
    static char kPowerSourceDescription[] = "CT Matter EVSE";
    esp_matter_attr_val_t val = esp_matter_char_str(kPowerSourceDescription, sizeof(kPowerSourceDescription) - 1);
    esp_err_t err = esp_matter::attribute::report(this->mEndpointId, PowerSource::Id,
                                                  PowerSource::Attributes::Description::Id, &val);
    PRINTF_DEBUG("Power Source Description: %s (%s)", (char *)(val.val.a.b), err == ESP_OK ? "OK" : "Failed");

    val = esp_matter_uint32(230'000);
    err = esp_matter::attribute::report(this->mEndpointId, PowerSource::Id,
                                        PowerSource::Attributes::WiredNominalVoltage::Id, &val);
    PRINTF_DEBUG("Power Source WiredNominalVoltage: %d (%s)", val.val.u32, err == ESP_OK ? "OK" : "Failed");

    val = esp_matter_uint32(32'000);
    err = esp_matter::attribute::report(this->mEndpointId, PowerSource::Id,
                                        PowerSource::Attributes::WiredMaximumCurrent::Id, &val);
    PRINTF_DEBUG("Power Source WiredMaximumCurrent: %d (%s)", val.val.u32, err == ESP_OK ? "OK" : "Failed");
}

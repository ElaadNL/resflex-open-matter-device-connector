// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ESP32DeviceInstanceInfoProvider.h"
#include "DeviceEnergyManagementDelegateImpl.h"
#include "ElectricalPowerMeasurementDelegate.h"
#include "EnergyEvseDelegateImpl.h"
#include "PowerTopologyDelegate_host.h"
#include "energy_evse_mode_host.h"

#include "chip_support.h"

namespace chip {
namespace app {
namespace Clusters {
namespace PowerSource {

enum class Feature : uint32_t {
    kWired = 0x01,
    kBattery = 0x02,
    kRechargeable = 0x04,
    kReplaceable = 0x08,
};

enum class WiredCurrentTypeEnum : uint8_t {
    kAc = 0,
};

namespace Attributes {
namespace Description {
constexpr AttributeId Id = 0x0000;
} // namespace Description
namespace WiredNominalVoltage {
constexpr AttributeId Id = 0x0001;
} // namespace WiredNominalVoltage
namespace WiredMaximumCurrent {
constexpr AttributeId Id = 0x0002;
} // namespace WiredMaximumCurrent
} // namespace Attributes

static constexpr ClusterId Id = 0x002F;

class PowerSourceDelegate {
public:
    PowerSourceDelegate() = default;

    void SetupDelegate(EndpointId id);
    void AddCustomAttributes();
    void AddCustomFeatures(BitMask<Feature, uint32_t> aFeature);
    void LateSetupAfterMatter();

    EndpointId mEndpointId = 0;
    BitMask<Feature, uint32_t> mFeature;

    struct config_t {
        struct {
            uint8_t wired_current_type = 0;
        } wired;
        struct {
        } battery;
        struct {
        } rechargeable;
        struct {
        } replaceable;
    } config;
};

} // namespace PowerSource
} // namespace Clusters
} // namespace app
} // namespace chip

namespace CT {
namespace Charger {

inline constexpr bool kSupportDischargingV2x = false;

class MatterManager {
public:
    static MatterManager &GetInstance();

    MatterManager(MatterManager const &) = delete;
    void operator=(MatterManager const &) = delete;

    void Init();
    void InitializeDeviceDelegates();
    void InitializeMatterDeviceNode();
    void StartMatterStack();

    static void Wrapper_MatterEventCb(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg);
    void HandleMatterEventCb(const chip::DeviceLayer::ChipDeviceEvent *event);

    void HandleMatterAttributeUpdate(uint16_t endpoint_id, uint32_t cluster_id, uint32_t attribute_id,
                                     esp_matter_attr_val_t *val);

    bool isConnected = false;
    esp_matter::node_t *device_node = nullptr;

    void UpdateState();
    void UpdateFaultState(uint8_t faultCode);

    void StartSession(int64_t currentEnergy);
    void StopSession(int64_t currentEnergy);
    void UpdateSession(int64_t currentEnergy);

    bool IsChargingAllowedByTargets(void);

    CHIP_ERROR InitializePowerMeasurementCluster();
    CHIP_ERROR InitializePowerSourceCluster();
    CHIP_ERROR SendReadings(int64_t aActivePower_mW, int64_t aVoltage_mV, int64_t aActiveCurrent_mA);
    CHIP_ERROR SendCumulativeEnergyReading(int64_t aCumulativeEnergyImported, int64_t aCumulativeEnergyExported);

    static void ReportAttributeChangeToMatter(chip::EndpointId endpoint, chip::ClusterId clusterId,
                                              chip::AttributeId attributeId);
    static int &ReportAttributeCallCount();

    bool GetChargingEnabled();

    void resetForTest();
    void SetChargingEnabledForTest(bool enabled);
    void SetTargetsAllowForTest(bool allowed);

    chip::DeviceLayer::EvseDeviceInstanceInfoProvider DIIProvider;

    chip::app::Clusters::EnergyEvse::EnergyEvseDelegate EE_dg;
    chip::app::Clusters::EnergyEvseMode::EnergyEvseModeDelegate EEM_dg;
    chip::app::Clusters::DeviceEnergyManagement::DeviceEnergyManagementDelegate DEM_dg;

    chip::app::Clusters::PowerSource::PowerSourceDelegate PS_dg;
    chip::app::Clusters::PowerTopology::PowerTopologyDelegate PT_dg;
    chip::app::Clusters::ElectricalPowerMeasurement::ElectricalPowerMeasurementDelegate EPM_dg;

private:
    MatterManager();
};

} // namespace Charger
} // namespace CT

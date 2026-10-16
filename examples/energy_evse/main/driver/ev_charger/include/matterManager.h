// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include <stdlib.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_matter.h"

#include <device.h>
#include <protocols/Protocols.h>

#include "helpers.h"

#include "ESP32DeviceInstanceInfoProvider.h"

#include "energy-evse-modes.h"
#include "DeviceEnergyManagementDelegateImpl.h"
#include "EnergyEvseDelegateImpl.h"
#include "PowerTopologyDelegate.h"
#include "ElectricalPowerMeasurementDelegate.h"

using chip::Protocols::InteractionModel::Status;

namespace chip {
namespace app {
namespace Clusters {
namespace PowerSource {

// using namespace esp_matter::cluster::power_source;
class PowerSourceDelegate {
private:
    /* data */
public:
    PowerSourceDelegate() = default;

    void SetupDelegate(chip::EndpointId id);
    void AddCustomAttributes();
    void AddCustomFeatures(Feature aFeature);
    void LateSetupAfterMatter();

    chip::EndpointId mEndpointId = 0;
    chip::BitMask<Feature, uint32_t> mFeature;

    using config_t = esp_matter::cluster::power_source::config_t;

    config_t config;
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

    bool isConnected;
    esp_matter::node_t *device_node;

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

    /* Delegates external APIs*/
    bool GetChargingEnabled();

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

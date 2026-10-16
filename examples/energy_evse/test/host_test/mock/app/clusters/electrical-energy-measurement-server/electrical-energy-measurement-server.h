// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "chip_support.h"

#include <map>

namespace chip {
namespace app {
namespace Clusters {
namespace ElectricalEnergyMeasurement {

namespace Structs {
namespace EnergyMeasurementStruct {
struct Type {
    Optional<uint32_t> startTimestamp;
    Optional<uint64_t> startSystime;
    Optional<uint32_t> endTimestamp;
    Optional<uint64_t> endSystime;
    int64_t energy = 0;

    void ClearValue() {}
};
} // namespace EnergyMeasurementStruct
} // namespace Structs

struct MeasurementData {
    Optional<Structs::EnergyMeasurementStruct::Type> cumulativeImported;
    Optional<Structs::EnergyMeasurementStruct::Type> cumulativeExported;
};

inline std::map<EndpointId, MeasurementData> &MeasurementStore()
{
    static std::map<EndpointId, MeasurementData> store;
    return store;
}

inline MeasurementData *MeasurementDataForEndpoint(EndpointId endpointId)
{
    auto it = MeasurementStore().find(endpointId);
    if (it == MeasurementStore().end()) {
        return nullptr;
    }
    return &it->second;
}

inline void unit_test_measurement_reset(void)
{
    MeasurementStore().clear();
}

inline bool &NotifyCumulativeEnergyFailForTest()
{
    static bool fail = false;
    return fail;
}

inline int &NotifyCumulativeEnergyCallCount()
{
    static int count = 0;
    return count;
}

inline bool NotifyCumulativeEnergyMeasured(EndpointId endpointId,
                                           const Optional<Structs::EnergyMeasurementStruct::Type> &energyImported,
                                           const Optional<Structs::EnergyMeasurementStruct::Type> &energyExported)
{
    ++NotifyCumulativeEnergyCallCount();
    if (NotifyCumulativeEnergyFailForTest()) {
        return false;
    }
    auto &data = MeasurementStore()[endpointId];
    if (energyImported.HasValue()) {
        data.cumulativeImported = energyImported;
    }
    if (energyExported.HasValue()) {
        data.cumulativeExported = energyExported;
    }
    return true;
}

} // namespace ElectricalEnergyMeasurement
} // namespace Clusters
} // namespace app
} // namespace chip

void MatterReportingAttributeChangeCallback(chip::EndpointId endpoint, chip::ClusterId clusterId,
                                            chip::AttributeId attributeId);

// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "app/clusters/device-energy-management-server/device-energy-management-server.h"

namespace chip {
namespace app {
namespace Clusters {
namespace DeviceEnergyManagement {

class DEMManufacturerDelegate {
public:
    DEMManufacturerDelegate() = default;
    virtual ~DEMManufacturerDelegate() = default;

    virtual int64_t GetApproxEnergyDuringSession() { return 0; }

    virtual CHIP_ERROR HandleDeviceEnergyManagementPowerAdjustRequest(int64_t, uint32_t, AdjustmentCauseEnum)
    {
        return CHIP_NO_ERROR;
    }
    virtual CHIP_ERROR HandleDeviceEnergyManagementPowerAdjustCompletion() { return CHIP_NO_ERROR; }
    virtual CHIP_ERROR HandleDeviceEnergyManagementCancelPowerAdjustRequest(CauseEnum) { return CHIP_NO_ERROR; }
    virtual CHIP_ERROR HandleDeviceEnergyManagementStartTimeAdjustRequest(uint32_t, AdjustmentCauseEnum)
    {
        return CHIP_NO_ERROR;
    }
    virtual CHIP_ERROR HandleDeviceEnergyManagementPauseRequest(uint32_t, AdjustmentCauseEnum) { return CHIP_NO_ERROR; }
    virtual CHIP_ERROR HandleDeviceEnergyManagementPauseCompletion() { return CHIP_NO_ERROR; }
    virtual CHIP_ERROR HandleDeviceEnergyManagementCancelPauseRequest(CauseEnum) { return CHIP_NO_ERROR; }
    virtual CHIP_ERROR HandleDeviceEnergyManagementCancelRequest() { return CHIP_NO_ERROR; }
    virtual CHIP_ERROR HandleModifyForecastRequest(
        uint32_t, const DataModel::DecodableList<Structs::SlotAdjustmentStruct::DecodableType> &, AdjustmentCauseEnum)
    {
        return CHIP_NO_ERROR;
    }
    virtual CHIP_ERROR RequestConstraintBasedForecast(
        const DataModel::DecodableList<Structs::ConstraintsStruct::DecodableType> &, AdjustmentCauseEnum)
    {
        return CHIP_NO_ERROR;
    }
};

} // namespace DeviceEnergyManagement
} // namespace Clusters
} // namespace app
} // namespace chip

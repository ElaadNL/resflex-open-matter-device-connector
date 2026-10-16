// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "chip_logging.h"
#include "app/clusters/energy-evse-server/energy-evse-server.h"
#include "app-common/zap-generated/cluster-objects.h"
#include "chip_support.h"

namespace chip {
namespace app {
namespace Clusters {
namespace EnergyEvse {

class ChargingTargetsMemMgr {
public:
    ChargingTargetsMemMgr();
    ~ChargingTargetsMemMgr();

    ChargingTargetsMemMgr(const ChargingTargetsMemMgr &) = delete;
    ChargingTargetsMemMgr &operator=(const ChargingTargetsMemMgr &) = delete;

    ChargingTargetsMemMgr(ChargingTargetsMemMgr &&other) noexcept;
    ChargingTargetsMemMgr &operator=(ChargingTargetsMemMgr &&other) noexcept;

    void PrepareDaySchedule(uint16_t chargingTargetSchedulesIdx);
    void AddChargingTarget(const EnergyEvse::Structs::ChargingTargetStruct::Type &chargingTarget);
    CHIP_ERROR AllocAndCopy();
    CHIP_ERROR AllocAndCopy(const DataModel::List<const Structs::ChargingTargetStruct::Type> &chargingTargets);
    CHIP_ERROR AllocAndCopy(
        const DataModel::DecodableList<Structs::ChargingTargetStruct::DecodableType> &chargingTargets);

    EnergyEvse::Structs::ChargingTargetStruct::Type *GetChargingTargets() const;
    uint16_t GetNumDailyChargingTargets() const;

private:
    EnergyEvse::Structs::ChargingTargetStruct::Type *mpListOfDays[kEvseTargetsMaxNumberOfDays];
    EnergyEvse::Structs::ChargingTargetStruct::Type mDailyChargingTargets[kEvseTargetsMaxTargetsPerDay];
    uint16_t mChargingTargetSchedulesIdx = 0;
    uint16_t mNumDailyChargingTargets = 0;
};

} // namespace EnergyEvse
} // namespace Clusters
} // namespace app
} // namespace chip

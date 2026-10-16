// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <app/clusters/energy-evse-server/energy-evse-server.h>
#include <credentials/FabricTable.h>
#include <lib/core/CHIPError.h>
#include <lib/core/CHIPPersistentStorageDelegate.h>
#include <lib/support/CHIPMem.h>
#include <lib/support/CodeUtils.h>
#include <lib/support/Pool.h>

#include <app-common/zap-generated/cluster-objects.h>

#include <ChargingTargetsMemMgr.h>

namespace chip {
namespace TLV {
class TLVReader;
} // namespace TLV
namespace app {
namespace Clusters {
namespace EnergyEvse {

class EvseTargetsDelegate : public chip::FabricTable::Delegate {
public:
    EvseTargetsDelegate() = default;
    ~EvseTargetsDelegate() override = default;

    CHIP_ERROR Init(PersistentStorageDelegate *targetStore);

    CHIP_ERROR LoadTargets();

    const DataModel::List<const Structs::ChargingTargetScheduleStruct::Type> &GetTargets() const;

    CHIP_ERROR SetTargets(const DataModel::DecodableList<Structs::ChargingTargetScheduleStruct::DecodableType>
                              &chargingTargetSchedulesChanges);

    CHIP_ERROR ClearTargets();

    void OnFabricRemoved(const FabricTable &fabricTable, FabricIndex fabricIndex) override;

private:
    static uint16_t GetTlvSizeUpperBound();

    CHIP_ERROR SaveTargets(
        DataModel::List<const Structs::ChargingTargetScheduleStruct::Type> &chargingTargetSchedulesList);

    void PrintTargets(
        const DataModel::List<const Structs::ChargingTargetScheduleStruct::Type> &chargingTargetSchedules) const;

protected:
    enum class TargetEntryTag : uint8_t {
        kTargetEntry = 1,
        kDayOfWeek = 2,
        kChargingTargetsList = 3,
        kChargingTargetsStruct = 4,
        kTargetTime = 5,
        kTargetSoC = 6,
        kAddedEnergy = 7,
    };

private:
    CHIP_ERROR DecodeChargingTargetStructFromTlv(::chip::TLV::TLVReader &reader,
                                                 Structs::ChargingTargetStruct::Type &outTarget);

    ChargingTargetsMemMgr mChargingTargets;

    Structs::ChargingTargetScheduleStruct::Type mChargingTargetSchedulesArray[kEvseTargetsMaxNumberOfDays];

    DataModel::List<const Structs::ChargingTargetScheduleStruct::Type> mChargingTargetSchedulesList;

    PersistentStorageDelegate *mpTargetStore = nullptr;

    static constexpr const char *spEvseTargetsKeyName = "g/ev/targ";
};

} // namespace EnergyEvse
} // namespace Clusters
} // namespace app
} // namespace chip

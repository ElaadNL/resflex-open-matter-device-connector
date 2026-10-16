// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "app-common/zap-generated/cluster-enums.h"
#include "app-common/zap-generated/cluster-objects.h"
#include "app/util/af-types.h"
#include "chip_support.h"
#include "protocols/interaction_model/StatusCode.h"

#include <cstdint>

namespace chip {
namespace app {
namespace Clusters {
namespace EnergyEvse {

static constexpr ClusterId Id = 0x00000099;

constexpr uint8_t kAllTargetDaysMask = 0x7f;

constexpr int64_t kMinimumChargeCurrent = 0;
constexpr uint32_t kMaxRandomizationDelayWindow = 86400;
constexpr uint8_t kEvseTargetsMaxNumberOfDays = 7;
constexpr uint8_t kEvseTargetsMaxTargetsPerDay = 10;

namespace Attributes {

namespace State {
static constexpr AttributeId Id = 0x0000;
}
namespace SupplyState {
static constexpr AttributeId Id = 0x0001;
}
namespace FaultState {
static constexpr AttributeId Id = 0x0002;
}
namespace ChargingEnabledUntil {
static constexpr AttributeId Id = 0x0003;
}
namespace DischargingEnabledUntil {
static constexpr AttributeId Id = 0x0004;
}
namespace CircuitCapacity {
static constexpr AttributeId Id = 0x0005;
}
namespace MinimumChargeCurrent {
static constexpr AttributeId Id = 0x0006;
}
namespace MaximumChargeCurrent {
static constexpr AttributeId Id = 0x0007;
}
namespace MaximumDischargeCurrent {
static constexpr AttributeId Id = 0x0008;
}
namespace UserMaximumChargeCurrent {
static constexpr AttributeId Id = 0x0009;
}
namespace RandomizationDelayWindow {
static constexpr AttributeId Id = 0x000A;
}
namespace NextChargeStartTime {
static constexpr AttributeId Id = 0x0023;
}
namespace NextChargeTargetTime {
static constexpr AttributeId Id = 0x0024;
}
namespace NextChargeRequiredEnergy {
static constexpr AttributeId Id = 0x0025;
}
namespace NextChargeTargetSoC {
static constexpr AttributeId Id = 0x0026;
}
namespace ApproximateEVEfficiency {
static constexpr AttributeId Id = 0x0027;
}
namespace StateOfCharge {
static constexpr AttributeId Id = 0x0030;
}
namespace BatteryCapacity {
static constexpr AttributeId Id = 0x0031;
}
namespace VehicleID {
static constexpr AttributeId Id = 0x0032;
}
namespace SessionID {
static constexpr AttributeId Id = 0x0040;
}
namespace SessionDuration {
static constexpr AttributeId Id = 0x0041;
}
namespace SessionEnergyCharged {
static constexpr AttributeId Id = 0x0042;
}
namespace SessionEnergyDischarged {
static constexpr AttributeId Id = 0x0043;
}

} // namespace Attributes

namespace Events {

namespace EVConnected {
struct Type {
    uint32_t sessionID = 0;
};
} // namespace EVConnected

namespace EVNotDetected {
struct Type {
    uint32_t sessionID = 0;
    StateEnum state = StateEnum::kNotPluggedIn;
    uint32_t sessionDuration = 0;
    int64_t sessionEnergyCharged = 0;
    Optional<int64_t> sessionEnergyDischarged;
};
} // namespace EVNotDetected

namespace EnergyTransferStarted {
struct Type {
    uint32_t sessionID = 0;
    StateEnum state = StateEnum::kNotPluggedIn;
    int64_t maximumCurrent = 0;
};
} // namespace EnergyTransferStarted

namespace EnergyTransferStopped {
struct Type {
    uint32_t sessionID = 0;
    StateEnum state = StateEnum::kNotPluggedIn;
    EnergyTransferStoppedReasonEnum reason = EnergyTransferStoppedReasonEnum::kEVSEStopped;
    int64_t energyTransferred = 0;
};
} // namespace EnergyTransferStopped

namespace Fault {
struct Type {
    DataModel::Nullable<uint32_t> sessionID;
    StateEnum state = StateEnum::kNotPluggedIn;
    FaultStateEnum faultStatePreviousState = FaultStateEnum::kNoError;
    FaultStateEnum faultStateCurrentState = FaultStateEnum::kNoError;
};
} // namespace Fault

namespace Rfid {
struct Type {
    ByteSpan uid;
};
} // namespace Rfid

} // namespace Events

class Delegate {
public:
    virtual ~Delegate() = default;

    void SetEndpointId(EndpointId aEndpoint) { mEndpointId = aEndpoint; }
    EndpointId GetEndpointId() const { return mEndpointId; }

    virtual Protocols::InteractionModel::Status Disable() = 0;

    virtual Protocols::InteractionModel::Status EnableCharging(const DataModel::Nullable<uint32_t> &enableChargeTime,
                                                               const int64_t &minimumChargeCurrent,
                                                               const int64_t &maximumChargeCurrent) = 0;

    virtual Protocols::InteractionModel::Status EnableDischarging(
        const DataModel::Nullable<uint32_t> &enableDischargeTime, const int64_t &maximumDischargeCurrent) = 0;

    virtual Protocols::InteractionModel::Status StartDiagnostics() = 0;

    virtual Protocols::InteractionModel::Status SetTargets(
        const DataModel::DecodableList<Structs::ChargingTargetScheduleStruct::DecodableType>
            &chargingTargetSchedules) = 0;

    virtual Protocols::InteractionModel::Status LoadTargets() = 0;

    virtual Protocols::InteractionModel::Status GetTargets(
        DataModel::List<const Structs::ChargingTargetScheduleStruct::Type> &chargingTargetSchedules) = 0;

    virtual Protocols::InteractionModel::Status ClearTargets() = 0;

    virtual StateEnum GetState() = 0;
    virtual SupplyStateEnum GetSupplyState() = 0;
    virtual FaultStateEnum GetFaultState() = 0;
    virtual DataModel::Nullable<uint32_t> GetChargingEnabledUntil() = 0;
    virtual DataModel::Nullable<uint32_t> GetDischargingEnabledUntil() = 0;
    virtual int64_t GetCircuitCapacity() = 0;
    virtual int64_t GetMinimumChargeCurrent() = 0;
    virtual int64_t GetMaximumChargeCurrent() = 0;
    virtual int64_t GetMaximumDischargeCurrent() = 0;
    virtual int64_t GetUserMaximumChargeCurrent() = 0;
    virtual uint32_t GetRandomizationDelayWindow() = 0;
    virtual DataModel::Nullable<uint32_t> GetNextChargeStartTime() = 0;
    virtual DataModel::Nullable<uint32_t> GetNextChargeTargetTime() = 0;
    virtual DataModel::Nullable<int64_t> GetNextChargeRequiredEnergy() = 0;
    virtual DataModel::Nullable<Percent> GetNextChargeTargetSoC() = 0;
    virtual DataModel::Nullable<uint16_t> GetApproximateEVEfficiency() = 0;
    virtual DataModel::Nullable<Percent> GetStateOfCharge() = 0;
    virtual DataModel::Nullable<int64_t> GetBatteryCapacity() = 0;
    virtual DataModel::Nullable<CharSpan> GetVehicleID() = 0;
    virtual DataModel::Nullable<uint32_t> GetSessionID() = 0;
    virtual DataModel::Nullable<uint32_t> GetSessionDuration() = 0;
    virtual DataModel::Nullable<int64_t> GetSessionEnergyCharged() = 0;
    virtual DataModel::Nullable<int64_t> GetSessionEnergyDischarged() = 0;

    virtual CHIP_ERROR SetUserMaximumChargeCurrent(int64_t aNewValue) = 0;
    virtual CHIP_ERROR SetRandomizationDelayWindow(uint32_t aNewValue) = 0;
    virtual CHIP_ERROR SetApproximateEVEfficiency(DataModel::Nullable<uint16_t> aNewValue) = 0;

protected:
    EndpointId mEndpointId = 0;
};

} // namespace EnergyEvse
} // namespace Clusters
} // namespace app
} // namespace chip

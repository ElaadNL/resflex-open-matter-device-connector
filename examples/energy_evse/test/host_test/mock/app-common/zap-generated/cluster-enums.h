// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "chip_support.h"

#include <cstdint>

namespace chip {
namespace app {
namespace Clusters {
namespace EnergyEvse {

enum class EnergyTransferStoppedReasonEnum : uint8_t {
    kEVStopped        = 0x00,
    kEVSEStopped      = 0x01,
    kOther            = 0x02,
    kUnknownEnumValue = 3,
};

enum class FaultStateEnum : uint8_t {
    kNoError           = 0x00,
    kMeterFailure      = 0x01,
    kOverVoltage       = 0x02,
    kUnderVoltage      = 0x03,
    kOverCurrent       = 0x04,
    kContactWetFailure = 0x05,
    kContactDryFailure = 0x06,
    kGroundFault       = 0x07,
    kPowerLoss         = 0x08,
    kPowerQuality      = 0x09,
    kPilotShortCircuit = 0x0A,
    kEmergencyStop     = 0x0B,
    kEVDisconnected    = 0x0C,
    kWrongPowerSupply  = 0x0D,
    kLiveNeutralSwap   = 0x0E,
    kOverTemperature   = 0x0F,
    kOther             = 0xFF,
    kUnknownEnumValue  = 16,
};

enum class StateEnum : uint8_t {
    kNotPluggedIn         = 0x00,
    kPluggedInNoDemand    = 0x01,
    kPluggedInDemand      = 0x02,
    kPluggedInCharging    = 0x03,
    kPluggedInDischarging = 0x04,
    kSessionEnding        = 0x05,
    kFault                = 0x06,
    kUnknownEnumValue     = 7,
};

enum class SupplyStateEnum : uint8_t {
    kDisabled            = 0x00,
    kChargingEnabled       = 0x01,
    kDischargingEnabled    = 0x02,
    kDisabledError         = 0x03,
    kDisabledDiagnostics   = 0x04,
    kEnabled               = 0x05,
    kUnknownEnumValue      = 6,
};

enum class Feature : uint32_t {
    kChargingPreferences = 0x1,
    kSoCReporting        = 0x2,
    kPlugAndCharge       = 0x4,
    kRfid                = 0x8,
    kV2x                 = 0x10,
};

enum class TargetDayOfWeekBitmap : uint8_t {
    kSunday    = 0x01,
    kMonday    = 0x02,
    kTuesday   = 0x04,
    kWednesday = 0x08,
    kThursday  = 0x10,
    kFriday    = 0x20,
    kSaturday  = 0x40,
};

} // namespace EnergyEvse
} // namespace Clusters
} // namespace app
} // namespace chip

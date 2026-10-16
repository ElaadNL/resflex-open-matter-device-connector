// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>

#include <app-common/zap-generated/cluster-enums.h>

namespace chip {
namespace app {
namespace Clusters {

enum class EVSECallbackType : uint8_t {
    StateChanged,
    ChargeCurrentChanged,
    ChargingPreferencesChanged,
    EnergyMeterReadingRequested,
    DeviceEnergyManagementChanged,
};

enum class ChargingDischargingType : uint8_t { kCharging, kDischarging };

struct EVSECbInfo {
    EVSECallbackType type;

    union {
        struct {
            EnergyEvse::StateEnum state;
            EnergyEvse::SupplyStateEnum supplyState;
        } StateChange;

        struct {
            int64_t maximumChargeCurrent;
        } ChargingCurrent;

        struct {
            ChargingDischargingType meterType;
            int64_t *energyMeterValuePtr;
        } EnergyMeterReadingRequest;
    };
};

typedef void (*EVSECallbackFunc)(const EVSECbInfo *cb, intptr_t arg);

struct EVSECallbackWrapper {
    EVSECallbackFunc handler;
    intptr_t arg;
};

} // namespace Clusters
} // namespace app
} // namespace chip

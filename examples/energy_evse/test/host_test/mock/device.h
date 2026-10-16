// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "chip_support.h"

#include <cstdint>

#define IP_EVENT 0
#define IP_EVENT_STA_GOT_IP 1

namespace chip {
namespace DeviceLayer {

namespace DeviceEventType {

enum : uint16_t {
    kInterfaceIpAddressChanged = 0x8000,
    kCommissioningComplete,
    kFailSafeTimerExpired,
    kCommissioningSessionStarted = 0x9000,
    kCommissioningSessionStopped,
    kCommissioningWindowOpened,
    kCommissioningWindowClosed,
    kFabricWillBeRemoved,
    kFabricRemoved,
    kFabricCommitted,
    kFabricUpdated,
    kESPSystemEvent = 0x9010,
};

} // namespace DeviceEventType

struct ChipDevicePlatformEvent {
    struct {
        int32_t Base = 0;
        int32_t Id = 0;
    } ESPSystemEvent;
};

struct ChipDeviceEvent {
    uint16_t Type = 0;

    union {
        ChipDevicePlatformEvent Platform;
    };
};

inline void SetDeviceInstanceInfoProvider(void *)
{
}

} // namespace DeviceLayer
} // namespace chip

// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "chip_energy_evse_enums.h"
#include "chip_support.h"

#include <time.h>

namespace chip {
namespace app {
namespace Clusters {
namespace DeviceEnergyManagement {

CHIP_ERROR GetEpochTS(uint32_t &chipEpoch);
BitMask<EnergyEvse::TargetDayOfWeekBitmap> GetLocalDayOfWeekFromUnixEpoch(time_t unixEpoch);
CHIP_ERROR GetLocalDayOfWeekNow(BitMask<EnergyEvse::TargetDayOfWeekBitmap> &dayOfWeekMap);
CHIP_ERROR GetMinutesPastMidnight(uint16_t &minutesPastMidnight);

} // namespace DeviceEnergyManagement
} // namespace Clusters
} // namespace app
} // namespace chip

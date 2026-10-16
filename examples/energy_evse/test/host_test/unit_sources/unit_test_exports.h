// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>

bool EnergyEvse_TargetSkippedAsPast(uint16_t targetMinutesPastMidnight, uint16_t minutesPastMidnightNow_m,
                                    bool allowTargetsInPast);

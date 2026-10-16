// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>

namespace CT {
namespace Charger {

/// Delivered session energy from cumulative meter reading minus session offset (mWh).
uint32_t session_energy_delivered_mWh(int64_t meter_mWh, int64_t offset_mWh);

/// Elapsed seconds from monotonic timestamps in microseconds (ESP `esp_timer_get_time` style).
int64_t session_time_elapsed_sec_from_monotonic_us(int64_t stop_us, int64_t start_us);

/// True when `thread_ticks` aligns to a whole multiple of `interval_sec` at `mgr_tick_period_ms`.
bool charger_thread_tick_aligns_interval(uint32_t thread_ticks, uint32_t interval_sec, uint32_t mgr_tick_period_ms);

} // namespace Charger
} // namespace CT

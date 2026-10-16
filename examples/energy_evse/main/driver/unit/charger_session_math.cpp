// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include "charger_session_math.h"

namespace CT {
namespace Charger {

uint32_t session_energy_delivered_mWh(int64_t meter_mWh, int64_t offset_mWh)
{
    return static_cast<uint32_t>(meter_mWh - offset_mWh);
}

int64_t session_time_elapsed_sec_from_monotonic_us(int64_t stop_us, int64_t start_us)
{
    return (stop_us - start_us) / 1000000LL;
}

bool charger_thread_tick_aligns_interval(uint32_t thread_ticks, uint32_t interval_sec, uint32_t mgr_tick_period_ms)
{
    return (thread_ticks % (interval_sec * 1000U / mgr_tick_period_ms) == 0U);
}

} // namespace Charger
} // namespace CT

// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "app/clusters/device-energy-management-server/device-energy-management-server.h"
#include "chip_support.h"

#include <cstdint>

namespace chip {
namespace app {

using EventNumber = uint64_t;

inline bool &LogEventFailForTest()
{
    static bool fail = false;
    return fail;
}

template <typename EventType>
CHIP_ERROR LogEvent(const EventType &event, EndpointId endpoint, EventNumber &eventNumber)
{
    (void)event;
    (void)endpoint;
    if (LogEventFailForTest()) {
        return CHIP_ERROR_INTERNAL;
    }
    eventNumber = 1;
    return CHIP_NO_ERROR;
}

} // namespace app
} // namespace chip

inline void unit_test_log_event_fail(bool fail)
{
    chip::app::LogEventFailForTest() = fail;
}

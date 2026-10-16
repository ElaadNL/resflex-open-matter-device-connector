// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "chip_support.h"

#include <cstdint>
#include <functional>
#include <vector>

namespace chip {
namespace DeviceLayer {
class SystemLayerImpl;
} // namespace DeviceLayer

namespace System {
namespace Clock {
using Seconds32 = int32_t;
} // namespace Clock

using Layer = DeviceLayer::SystemLayerImpl;
using TimerCompleteCallback = void (*)(Layer *systemLayer, void *appState);
} // namespace System

namespace DeviceLayer {

class SystemLayerImpl {
public:
    static SystemLayerImpl &Get();

    CHIP_ERROR StartTimer(System::Clock::Seconds32 delay, System::TimerCompleteCallback onComplete, void *appState);
    void CancelTimer(System::TimerCompleteCallback onComplete, void *appState);

    void ScheduleLambda(const std::function<void()> &lambda);

    void FireExpiredTimersForTest();
    void ResetForTest();

    void SetStartTimerFailForTest(bool fail) { start_timer_fail_ = fail; }

private:
    struct Entry {
        System::Clock::Seconds32 delay;
        System::TimerCompleteCallback callback = nullptr;
        void *context = nullptr;
    };

    std::vector<Entry> timers_;
    std::vector<std::function<void()>> lambdas_;
    bool start_timer_fail_ = false;
};

// Matches CHIP PlatformManager: DeviceLayer::SystemLayer()
inline SystemLayerImpl &SystemLayer()
{
    return SystemLayerImpl::Get();
}

} // namespace DeviceLayer

} // namespace chip

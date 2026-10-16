// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include "chip_system_layer.h"

namespace chip {
namespace DeviceLayer {

SystemLayerImpl &SystemLayerImpl::Get()
{
    static SystemLayerImpl layer;
    return layer;
}

CHIP_ERROR SystemLayerImpl::StartTimer(System::Clock::Seconds32 delay, System::TimerCompleteCallback onComplete,
                                       void *appState)
{
    if (start_timer_fail_) {
        return CHIP_ERROR_INTERNAL;
    }
    timers_.push_back(Entry{delay, onComplete, appState});
    return CHIP_NO_ERROR;
}

void SystemLayerImpl::CancelTimer(System::TimerCompleteCallback onComplete, void *appState)
{
    for (auto it = timers_.begin(); it != timers_.end();) {
        if (it->callback == onComplete && it->context == appState) {
            it = timers_.erase(it);
        } else {
            ++it;
        }
    }
}

void SystemLayerImpl::ScheduleLambda(const std::function<void()> &lambda)
{
    lambdas_.push_back(lambda);
}

void SystemLayerImpl::FireExpiredTimersForTest()
{
    const auto pending = timers_;
    timers_.clear();
    for (const auto &entry : pending) {
        if (entry.callback != nullptr) {
            entry.callback(this, entry.context);
        }
    }

    const auto pendingLambdas = lambdas_;
    lambdas_.clear();
    for (const auto &fn : pendingLambdas) {
        if (fn) {
            fn();
        }
    }
}

void SystemLayerImpl::ResetForTest()
{
    timers_.clear();
    lambdas_.clear();
    start_timer_fail_ = false;
}

} // namespace DeviceLayer
} // namespace chip

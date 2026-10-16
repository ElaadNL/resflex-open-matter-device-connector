// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <cstring>

#include "esp_err.h"

typedef void *esp_timer_handle_t;
typedef void (*esp_timer_cb_t)(void *);

struct esp_timer_create_args_t {
    esp_timer_cb_t callback;
    void *arg;
    int dispatch_method;
    const char *name;
};

#define ESP_TIMER_TASK 0

/// Host monotonic clock (microseconds).
inline int64_t &unit_test_esp_timer_now_us()
{
    static int64_t v = 1'000'000LL;
    return v;
}

inline int64_t esp_timer_get_time(void)
{
    return unit_test_esp_timer_now_us();
}

struct UnitTestEspTimerState {
    esp_timer_cb_t callback = nullptr;
    void *arg = nullptr;
    bool running = false;
    uint64_t period_us = 0;
};

inline UnitTestEspTimerState &unit_test_esp_timer_state()
{
    static UnitTestEspTimerState s;
    return s;
}

inline void unit_test_esp_timer_reset()
{
    unit_test_esp_timer_state() = {};
}

inline void unit_test_esp_timer_fire_once()
{
    auto &s = unit_test_esp_timer_state();
    if (s.callback != nullptr) {
        s.callback(s.arg);
    }
}

inline int esp_timer_create(const esp_timer_create_args_t *args, esp_timer_handle_t *out_handle)
{
    if (args == nullptr || out_handle == nullptr) {
        return -1;
    }
    auto &s = unit_test_esp_timer_state();
    s.callback = args->callback;
    s.arg = args->arg;
    *out_handle = reinterpret_cast<esp_timer_handle_t>(1);
    return ESP_OK;
}

inline int esp_timer_start_periodic(esp_timer_handle_t handle, uint64_t period_us)
{
    (void)handle;
    auto &s = unit_test_esp_timer_state();
    s.running = true;
    s.period_us = period_us;
    return ESP_OK;
}

inline int esp_timer_stop(esp_timer_handle_t handle)
{
    (void)handle;
    unit_test_esp_timer_state().running = false;
    return ESP_OK;
}

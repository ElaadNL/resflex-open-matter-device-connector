// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include "hardwareControlInterface.h"

#include <cmath>

#ifndef UNIT_TEST
#include "esp_log.h"
#include "esp_timer.h"
#include <esp_random.h>
#else
#include <cstring>
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#endif

namespace CT {
namespace Charger {

namespace {

static const char *const TAG = "hw_ctrl";

constexpr uint32_t kNominalVoltage_mV = 230'000;
constexpr uint32_t kBaseCurrent_mA = 32'000;

uint32_t fakeCurrentBaseOnTarget(uint32_t targetCurrent_mA)
{
    int32_t offset_uA = (int32_t)(esp_random() % 1001) - 500;
    uint32_t current = targetCurrent_mA + offset_uA;

    // ESP_LOGI(TAG, "Generated fake current: %u mA (target: %u mA, offset: %d uA)", current, targetCurrent_mA,
    // offset_uA);
    return current;
}

} // namespace

void HardwareControlInterface::meterTimerCallback(void *callbackContext)
{
    if (callbackContext == nullptr) {
        ESP_LOGE(TAG, "meterTimerCallback received null callback context");
        return;
    }

    auto *interface = static_cast<HardwareControlInterface *>(callbackContext);
    invokeMeterTimerOnInterface(interface);
}

void HardwareControlInterface::invokeMeterTimerOnInterface(HardwareControlInterface *hardwareControlIface)
{
    hardwareControlIface->onMeterTimer();
}

HardwareControlInterface &HardwareControlInterface::Instance()
{
    static HardwareControlInterface inst;
    return inst;
}

esp_err_t HardwareControlInterface::init()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (inited_) {
        return ESP_OK;
    }

    state_.cable = HwCableStatus_t::NOT_CONNECTED;
    state_.chargeCommand = HwChargeCommand_t::STOP;
    state_.meter = {};
    state_.limit = {.current_mA = kBaseCurrent_mA};

    esp_timer_create_args_t cfg = {};
    cfg.callback = &HardwareControlInterface::meterTimerCallback;
    cfg.arg = this;
    cfg.dispatch_method = ESP_TIMER_TASK;
    cfg.name = "hw_meter";

    esp_timer_handle_t t = nullptr;
    esp_err_t err = esp_timer_create(&cfg, &t);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_timer_create failed: %s", esp_err_to_name(err));
        return err;
    }
    timer_handle_ = t;

    inited_ = true;
    return ESP_OK;
}

// Add a member variable: bool is_paused_ = false;

void HardwareControlInterface::startMeterTimer()
{
    if (!inited_ || timer_handle_ == nullptr) {
        return;
    }

    auto t = static_cast<esp_timer_handle_t>(timer_handle_);

    // If already paused, just resume
    if (is_paused_) {
        resumeMeterTimer();
        return;
    }

    esp_timer_stop(t);
    const esp_err_t err = esp_timer_start_periodic(t, 1'000'000ULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_timer_start_periodic failed: %s", esp_err_to_name(err));
    }
    is_paused_ = false;
}

void HardwareControlInterface::pauseMeterTimer()
{
    if (timer_handle_ == nullptr || is_paused_) {
        return;
    }

    esp_timer_stop(static_cast<esp_timer_handle_t>(timer_handle_));
    is_paused_ = true;
    ESP_LOGI(TAG, "Timer paused");
}

void HardwareControlInterface::resumeMeterTimer()
{
    if (timer_handle_ == nullptr || !is_paused_) {
        return;
    }

    auto t = static_cast<esp_timer_handle_t>(timer_handle_);
    // Restarting periodic timer from where it left off (approx)
    const esp_err_t err = esp_timer_start_periodic(t, 1'000'000ULL);
    if (err == ESP_OK) {
        is_paused_ = false;
        ESP_LOGI(TAG, "Timer resumed");
    } else {
        ESP_LOGE(TAG, "Failed to resume timer: %s", esp_err_to_name(err));
    }
}

void HardwareControlInterface::stopMeterTimer()
{
    if (timer_handle_ == nullptr) {
        return;
    }
    esp_timer_stop(static_cast<esp_timer_handle_t>(timer_handle_));
    is_paused_ = false; // Reset state on full stop
}

void HardwareControlInterface::onMeterTimer()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_.chargeCommand != HwChargeCommand_t::START) {
        return;
    }

    static uint32_t tick = 0;
    ++tick;

    state_.meter.voltage_mV = kNominalVoltage_mV;
    state_.meter.current_mA = is_EV_drawing_ ? fakeCurrentBaseOnTarget(state_.limit.current_mA) : 0;
    state_.meter.power_mW =
        static_cast<uint32_t>((static_cast<uint64_t>(state_.meter.voltage_mV) * state_.meter.current_mA) / 1'000'000);
    state_.meter.energy_mWh += state_.meter.power_mW / 3'600;

    // ESP_LOGI(TAG, "Meter update - Voltage: %.1f V, Current: %.1f A, Power: %.1f W, Energy: %.2f Wh",
    //          state_.meter.voltage_mV / 1000.0, state_.meter.current_mA / 1000.0, state_.meter.power_mW / 1000.0,
    //          state_.meter.energy_mWh / 1000.0);
}

void HardwareControlInterface::setCableStatus(HwCableStatus_t s)
{
    std::lock_guard<std::mutex> lock(mutex_);
    state_.cable = s;

    PRINTF_DEBUG("emulator: cable status = %s", s == HwCableStatus_t::CONNECTED ? "CONNECTED" : "NOT_CONNECTED");
}

HwCableStatus_t HardwareControlInterface::getCableStatus() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return state_.cable;
}

void HardwareControlInterface::setChargeCommand(HwChargeCommand_t cmd)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_.chargeCommand = cmd;
    }

    if (cmd == HwChargeCommand_t::START) {
        is_charging_ = true;
        is_EV_drawing_ = true;
        startMeterTimer();
    } else if (cmd == HwChargeCommand_t::PAUSE) {
        pauseMeterTimer();
    } else if (cmd == HwChargeCommand_t::STOP) {
        is_charging_ = false;
        is_EV_drawing_ = false;
        stopMeterTimer();
    }
}

HwChargeCommand_t HardwareControlInterface::getChargeCommand() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return state_.chargeCommand;
}

void HardwareControlInterface::setMeter(const HwChargingMeter_t &m)
{
    std::lock_guard<std::mutex> lock(mutex_);
    state_.meter = m;
}

HwChargingMeter_t HardwareControlInterface::getMeter() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return state_.meter;
}

void HardwareControlInterface::setLimit(const HwChargingLimit_t &l)
{
    std::lock_guard<std::mutex> lock(mutex_);
    state_.limit = l;
}

HwChargingLimit_t HardwareControlInterface::getLimit() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return state_.limit;
}

HwControlState_t HardwareControlInterface::getStateSnapshot() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return state_;
}

esp_err_t HardwareControlInterface::getParam(HwControlParam_t p, int64_t *out) const
{
    if (out == nullptr || p >= HwControlParam_t::PARAM_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    switch (p) {
    case HwControlParam_t::CABLE_STATUS:
        *out = static_cast<int64_t>(state_.cable);
        break;
    case HwControlParam_t::CHARGE_LIMIT_MA:
        *out = state_.limit.current_mA;
        break;
    case HwControlParam_t::METER_CURRENT_MA:
        *out = state_.meter.current_mA;
        break;
    case HwControlParam_t::METER_VOLTAGE_MV:
        *out = state_.meter.voltage_mV;
        break;
    case HwControlParam_t::METER_POWER_MW:
        *out = state_.meter.power_mW;
        break;
    case HwControlParam_t::METER_ENERGY_MWH:
        *out = state_.meter.energy_mWh;
        break;
    case HwControlParam_t::CHARGE_COMMAND:
        *out = static_cast<int64_t>(state_.chargeCommand);
        break;
    default:
        return ESP_ERR_NOT_SUPPORTED;
    }
    return ESP_OK;
}

esp_err_t HardwareControlInterface::setParam(HwControlParam_t p, int64_t v)
{
    if (p >= HwControlParam_t::PARAM_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }

    if (p == HwControlParam_t::CHARGE_COMMAND) {
        const auto cmd = static_cast<HwChargeCommand_t>(v);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            state_.chargeCommand = cmd;
        }
        if (cmd == HwChargeCommand_t::START) {
            startMeterTimer();
        } else if (cmd == HwChargeCommand_t::PAUSE) {
            pauseMeterTimer();
        } else if (cmd == HwChargeCommand_t::STOP) {
            stopMeterTimer();
        } else {
            return ESP_ERR_NOT_SUPPORTED;
        }
        return ESP_OK;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    switch (p) {
    case HwControlParam_t::CABLE_STATUS:
        state_.cable = static_cast<HwCableStatus_t>(v);
        break;
    case HwControlParam_t::METER_CURRENT_MA:
        state_.meter.current_mA = static_cast<int32_t>(v);
        break;
    case HwControlParam_t::METER_VOLTAGE_MV:
        state_.meter.voltage_mV = static_cast<int32_t>(v);
        break;
    case HwControlParam_t::METER_POWER_MW:
        state_.meter.power_mW = static_cast<int32_t>(v);
        break;
    case HwControlParam_t::METER_ENERGY_MWH:
        state_.meter.energy_mWh = v;
        break;
    default:
        return ESP_ERR_NOT_SUPPORTED;
    }
    return ESP_OK;
}

#ifdef UNIT_TEST
void HardwareControlInterface::resetForTest()
{
    stopMeterTimer();
    std::lock_guard<std::mutex> lock(mutex_);
    state_ = {};
    state_.limit = {.current_mA = kBaseCurrent_mA};
    is_paused_ = false;
    is_charging_ = false;
    is_EV_drawing_ = false;
    faultCode_ = 0;
    inited_ = false;
    timer_handle_ = nullptr;
    unit_test_esp_timer_reset();
    unit_test_esp_random_next() = 500U;
}

void HardwareControlInterface::setChargingForTest(bool charging)
{
    std::lock_guard<std::mutex> lock(mutex_);
    is_charging_ = charging;
}

void HardwareControlInterface::invokeMeterTimerForTest()
{
    onMeterTimer();
}

void HardwareControlInterface::InvokeMeterTimerCallbackForTest(void *callbackContext)
{
    meterTimerCallback(callbackContext);
}
#endif

} // namespace Charger
} // namespace CT

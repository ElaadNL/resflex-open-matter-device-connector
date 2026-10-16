// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <mutex>

#include "esp_err.h"

#include "helpers.h"

namespace CT {
namespace Charger {

/** Physical cable sense at the hardware abstraction layer. */
enum class HwCableStatus_t : uint8_t {
    NOT_CONNECTED = 0,
    CONNECTED = 1,
};

/** Start/stop relay or pilot request from the hardware control plane. */
enum class HwChargeCommand_t : uint8_t {
    STOP = 0,
    START = 1,
    PAUSE = 2,
};

/**
 * Enumerated hardware-control parameters for generic get/set.
 * Values are carried as int64_t; enums are cast to/from integer.
 */
enum class HwControlParam_t : uint8_t {
    CABLE_STATUS = 0,
    CHARGE_LIMIT_MA,
    METER_CURRENT_MA,
    METER_VOLTAGE_MV,
    METER_POWER_MW,
    METER_ENERGY_MWH,
    CHARGE_COMMAND,
    PARAM_COUNT
};

struct HwChargingMeter_t {
    uint32_t current_mA;
    uint32_t voltage_mV;
    uint32_t power_mW;
    uint64_t energy_mWh;
};

struct HwChargingLimit_t {
    int32_t current_mA;
};

struct HwControlState_t {
    HwCableStatus_t cable;
    HwChargingLimit_t limit;
    HwChargeCommand_t chargeCommand;
    HwChargingMeter_t meter;
};

/**
 * Emulated hardware control interface: cable sense, meter, and charge on/off.
 * When charge command is START, meter readings advance on a 1 Hz timer.
 */
class HardwareControlInterface {
public:
    static HardwareControlInterface &Instance();

    HardwareControlInterface(const HardwareControlInterface &) = delete;
    HardwareControlInterface &operator=(const HardwareControlInterface &) = delete;

    esp_err_t init();
    void resetMeterData()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_.meter = {};
    }

    void setCableStatus(HwCableStatus_t s);
    HwCableStatus_t getCableStatus() const;

    void setChargeCommand(HwChargeCommand_t cmd);
    HwChargeCommand_t getChargeCommand() const;

    void setMeter(const HwChargingMeter_t &m);
    HwChargingMeter_t getMeter() const;

    void setLimit(const HwChargingLimit_t &m);
    HwChargingLimit_t getLimit() const;

    void setFaultCode(uint8_t code)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        faultCode_ = code;
    }
    uint8_t getFaultCode() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return faultCode_;
    }

    void setEVDrawing(bool drawing)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        is_EV_drawing_ = drawing;
    }
    bool isEVDrawing() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return is_EV_drawing_;
    }

    bool isCharging() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return is_charging_;
    }

    HwControlState_t getStateSnapshot() const;

    esp_err_t getParam(HwControlParam_t p, int64_t *out) const;
    esp_err_t setParam(HwControlParam_t p, int64_t v);

#ifdef UNIT_TEST
    void resetForTest();
    void setChargingForTest(bool charging);
    void invokeMeterTimerForTest();
    static void InvokeMeterTimerCallbackForTest(void *callbackContext);
#endif

private:
    HardwareControlInterface() = default;

    static void meterTimerCallback(void *callbackContext);
    static void invokeMeterTimerOnInterface(HardwareControlInterface *hardwareControlIface);

    void startMeterTimer();
    void stopMeterTimer();
    void resumeMeterTimer();
    void pauseMeterTimer();
    void onMeterTimer();

    mutable std::mutex mutex_;
    HwControlState_t state_{};
    bool inited_ = false;
    bool is_paused_ = false;
    bool is_charging_ = false;
    bool is_EV_drawing_ = false;
    uint8_t faultCode_ = 0;

    void *timer_handle_ = nullptr;
};

} // namespace Charger
} // namespace CT

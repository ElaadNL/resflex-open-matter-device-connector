// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include "hardwareControlInterface.h"

#include "esp_random.h"
#include "esp_timer.h"

#include "unity.h"

using CT::Charger::HardwareControlInterface;
using CT::Charger::HwCableStatus_t;
using CT::Charger::HwChargeCommand_t;
using CT::Charger::HwChargingLimit_t;
using CT::Charger::HwChargingMeter_t;
using CT::Charger::HwControlParam_t;

static HardwareControlInterface &Hw()
{
    return HardwareControlInterface::Instance();
}

static void hw_reset()
{
    Hw().resetForTest();
}

static void test_hw_init_idempotent(void)
{
    hw_reset();
    TEST_ASSERT_EQUAL_INT(ESP_OK, Hw().init());
    TEST_ASSERT_EQUAL_INT(ESP_OK, Hw().init());
}

static void test_hw_cable_status_roundtrip(void)
{
    hw_reset();
    Hw().setCableStatus(HwCableStatus_t::CONNECTED);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(HwCableStatus_t::CONNECTED), static_cast<int>(Hw().getCableStatus()));
    Hw().setCableStatus(HwCableStatus_t::NOT_CONNECTED);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(HwCableStatus_t::NOT_CONNECTED), static_cast<int>(Hw().getCableStatus()));
}

static void test_hw_limit_and_meter_roundtrip(void)
{
    hw_reset();
    Hw().setLimit(HwChargingLimit_t{.current_mA = 16000});
    TEST_ASSERT_EQUAL_INT(16000, Hw().getLimit().current_mA);

    HwChargingMeter_t m{};
    m.current_mA = 8000;
    m.voltage_mV = 230000;
    m.power_mW = 1840000;
    m.energy_mWh = 12000;
    Hw().setMeter(m);
    HwChargingMeter_t out = Hw().getMeter();
    TEST_ASSERT_EQUAL_UINT32(8000U, out.current_mA);
    TEST_ASSERT_EQUAL_UINT32(230000U, out.voltage_mV);
    TEST_ASSERT_EQUAL_UINT32(1840000U, out.power_mW);
    TEST_ASSERT_EQUAL_UINT64(12000ULL, out.energy_mWh);
}

static void test_hw_state_snapshot(void)
{
    hw_reset();
    Hw().setCableStatus(HwCableStatus_t::CONNECTED);
    Hw().setChargeCommand(HwChargeCommand_t::START);
    auto snap = Hw().getStateSnapshot();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(HwCableStatus_t::CONNECTED), static_cast<int>(snap.cable));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(HwChargeCommand_t::START), static_cast<int>(snap.chargeCommand));
}

static void test_hw_charge_command_start_pause_stop(void)
{
    hw_reset();
    TEST_ASSERT_EQUAL_INT(ESP_OK, Hw().init());

    Hw().setChargeCommand(HwChargeCommand_t::START);
    TEST_ASSERT_TRUE(Hw().isCharging());
    TEST_ASSERT_TRUE(Hw().isEVDrawing());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(HwChargeCommand_t::START), static_cast<int>(Hw().getChargeCommand()));

    Hw().setChargeCommand(HwChargeCommand_t::PAUSE);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(HwChargeCommand_t::PAUSE), static_cast<int>(Hw().getChargeCommand()));

    Hw().setChargeCommand(HwChargeCommand_t::STOP);
    TEST_ASSERT_FALSE(Hw().isCharging());
    TEST_ASSERT_FALSE(Hw().isEVDrawing());
}

static void test_hw_fault_and_ev_drawing(void)
{
    hw_reset();
    Hw().setFaultCode(7);
    TEST_ASSERT_EQUAL_UINT8(7U, Hw().getFaultCode());
    Hw().setEVDrawing(false);
    TEST_ASSERT_FALSE(Hw().isEVDrawing());
}

static void test_hw_getParam_invalid_and_all_fields(void)
{
    hw_reset();
    Hw().setCableStatus(HwCableStatus_t::CONNECTED);
    Hw().setLimit(HwChargingLimit_t{.current_mA = 10000});
    HwChargingMeter_t m{};
    m.current_mA = 9000;
    m.voltage_mV = 230000;
    m.power_mW = 2000000;
    m.energy_mWh = 5000;
    Hw().setMeter(m);
    Hw().setChargeCommand(HwChargeCommand_t::START);

    int64_t v = 0;
    TEST_ASSERT_EQUAL_INT(ESP_ERR_INVALID_ARG, Hw().getParam(HwControlParam_t::CABLE_STATUS, nullptr));
    TEST_ASSERT_EQUAL_INT(ESP_ERR_INVALID_ARG, Hw().getParam(HwControlParam_t::PARAM_COUNT, &v));

    TEST_ASSERT_EQUAL_INT(ESP_OK, Hw().getParam(HwControlParam_t::CABLE_STATUS, &v));
    TEST_ASSERT_EQUAL_INT64(static_cast<int64_t>(HwCableStatus_t::CONNECTED), v);
    TEST_ASSERT_EQUAL_INT(ESP_OK, Hw().getParam(HwControlParam_t::CHARGE_LIMIT_MA, &v));
    TEST_ASSERT_EQUAL_INT64(10000, v);
    TEST_ASSERT_EQUAL_INT(ESP_OK, Hw().getParam(HwControlParam_t::METER_CURRENT_MA, &v));
    TEST_ASSERT_EQUAL_INT64(9000, v);
    TEST_ASSERT_EQUAL_INT(ESP_OK, Hw().getParam(HwControlParam_t::METER_VOLTAGE_MV, &v));
    TEST_ASSERT_EQUAL_INT64(230000, v);
    TEST_ASSERT_EQUAL_INT(ESP_OK, Hw().getParam(HwControlParam_t::METER_POWER_MW, &v));
    TEST_ASSERT_EQUAL_INT64(2000000, v);
    TEST_ASSERT_EQUAL_INT(ESP_OK, Hw().getParam(HwControlParam_t::METER_ENERGY_MWH, &v));
    TEST_ASSERT_EQUAL_INT64(5000, v);
    TEST_ASSERT_EQUAL_INT(ESP_OK, Hw().getParam(HwControlParam_t::CHARGE_COMMAND, &v));
    TEST_ASSERT_EQUAL_INT64(static_cast<int64_t>(HwChargeCommand_t::START), v);
}

static void test_hw_setParam_charge_commands_and_fields(void)
{
    hw_reset();
    TEST_ASSERT_EQUAL_INT(ESP_OK, Hw().init());

    TEST_ASSERT_EQUAL_INT(ESP_ERR_INVALID_ARG, Hw().setParam(HwControlParam_t::PARAM_COUNT, 0));

    TEST_ASSERT_EQUAL_INT(
        ESP_OK, Hw().setParam(HwControlParam_t::CHARGE_COMMAND, static_cast<int64_t>(HwChargeCommand_t::START)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(HwChargeCommand_t::START), static_cast<int>(Hw().getChargeCommand()));

    TEST_ASSERT_EQUAL_INT(
        ESP_OK, Hw().setParam(HwControlParam_t::CHARGE_COMMAND, static_cast<int64_t>(HwChargeCommand_t::PAUSE)));
    TEST_ASSERT_EQUAL_INT(
        ESP_OK, Hw().setParam(HwControlParam_t::CHARGE_COMMAND, static_cast<int64_t>(HwChargeCommand_t::STOP)));

    TEST_ASSERT_EQUAL_INT(
        ESP_OK, Hw().setParam(HwControlParam_t::CABLE_STATUS, static_cast<int64_t>(HwCableStatus_t::CONNECTED)));
    TEST_ASSERT_EQUAL_INT(ESP_OK, Hw().setParam(HwControlParam_t::METER_CURRENT_MA, 12000));
    TEST_ASSERT_EQUAL_INT(ESP_OK, Hw().setParam(HwControlParam_t::METER_VOLTAGE_MV, 240000));
    TEST_ASSERT_EQUAL_INT(ESP_OK, Hw().setParam(HwControlParam_t::METER_POWER_MW, 2500000));
    TEST_ASSERT_EQUAL_INT(ESP_OK, Hw().setParam(HwControlParam_t::METER_ENERGY_MWH, 9999));

    int64_t v = 0;
    TEST_ASSERT_EQUAL_INT(ESP_OK, Hw().getParam(HwControlParam_t::METER_ENERGY_MWH, &v));
    TEST_ASSERT_EQUAL_INT64(9999, v);

    TEST_ASSERT_EQUAL_INT(ESP_ERR_NOT_SUPPORTED, Hw().setParam(HwControlParam_t::CHARGE_COMMAND, 99));
    TEST_ASSERT_EQUAL_INT(ESP_ERR_NOT_SUPPORTED, Hw().setParam(HwControlParam_t::CHARGE_LIMIT_MA, 8000));
}

static void test_hw_meter_timer_callback_null_context(void)
{
    hw_reset();
    HardwareControlInterface::InvokeMeterTimerCallbackForTest(nullptr);
}

static void test_hw_meter_timer_via_callback_updates_energy(void)
{
    hw_reset();
    TEST_ASSERT_EQUAL_INT(ESP_OK, Hw().init());
    unit_test_esp_random_next() = 500U;
    Hw().setLimit(HwChargingLimit_t{.current_mA = 32000});
    Hw().setChargeCommand(HwChargeCommand_t::START);

    unit_test_esp_timer_fire_once();
    HwChargingMeter_t m = Hw().getMeter();
    TEST_ASSERT_EQUAL_UINT32(230000U, m.voltage_mV);
    TEST_ASSERT_EQUAL_UINT32(32000U, m.current_mA);
    TEST_ASSERT_TRUE(m.power_mW > 0U);
    TEST_ASSERT_TRUE(m.energy_mWh > 0U);
}

static void test_hw_on_meter_timer_skips_when_not_start(void)
{
    hw_reset();
    Hw().setChargeCommand(HwChargeCommand_t::STOP);
    Hw().invokeMeterTimerForTest();
    HwChargingMeter_t m = Hw().getMeter();
    TEST_ASSERT_EQUAL_UINT32(0U, m.energy_mWh);
}

static void test_hw_on_meter_timer_zero_current_when_ev_not_drawing(void)
{
    hw_reset();
    TEST_ASSERT_EQUAL_INT(ESP_OK, Hw().init());
    Hw().setChargeCommand(HwChargeCommand_t::START);
    Hw().setEVDrawing(false);
    Hw().invokeMeterTimerForTest();
    TEST_ASSERT_EQUAL_UINT32(0U, Hw().getMeter().current_mA);
}

static void test_hw_reset_meter_data(void)
{
    hw_reset();
    HwChargingMeter_t m{};
    m.energy_mWh = 42;
    Hw().setMeter(m);
    Hw().resetMeterData();
    TEST_ASSERT_EQUAL_UINT64(0ULL, Hw().getMeter().energy_mWh);
}

void run_test_charger_hardware_control_tests(void)
{
    RUN_TEST(test_hw_init_idempotent);
    RUN_TEST(test_hw_cable_status_roundtrip);
    RUN_TEST(test_hw_limit_and_meter_roundtrip);
    RUN_TEST(test_hw_state_snapshot);
    RUN_TEST(test_hw_charge_command_start_pause_stop);
    RUN_TEST(test_hw_fault_and_ev_drawing);
    RUN_TEST(test_hw_getParam_invalid_and_all_fields);
    RUN_TEST(test_hw_setParam_charge_commands_and_fields);
    RUN_TEST(test_hw_meter_timer_callback_null_context);
    RUN_TEST(test_hw_meter_timer_via_callback_updates_energy);
    RUN_TEST(test_hw_on_meter_timer_skips_when_not_start);
    RUN_TEST(test_hw_on_meter_timer_zero_current_when_ev_not_drawing);
    RUN_TEST(test_hw_reset_meter_data);
}

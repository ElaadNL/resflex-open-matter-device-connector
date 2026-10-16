// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include "esp_timer.h"
#include "chargerManager.h"

#include "unity.h"

using CT::Charger::AuthType_t;
using CT::Charger::ChargerManager;
using CT::Charger::ChargerStatus_t;
using CT::Charger::HardwareControlInterface;
using CT::Charger::HwCableStatus_t;
using CT::Charger::HwChargeCommand_t;
using CT::Charger::HwChargingLimit_t;
using CT::Charger::HwChargingMeter_t;
using CT::Charger::MatterManager;

static ChargerManager &M()
{
    return ChargerManager::Controller();
}

static HardwareControlInterface &Hw()
{
    return HardwareControlInterface::Instance();
}

static void mgr_reset()
{
    M().resetForTest();
}

static void test_mgr_statusToString_all_and_unknown(void)
{
    mgr_reset();
    TEST_ASSERT_EQUAL_STRING("AVAILABLE", M().statusToString(ChargerStatus_t::AVAILABLE).c_str());
    TEST_ASSERT_EQUAL_STRING("PREPARING", M().statusToString(ChargerStatus_t::PREPARING).c_str());
    TEST_ASSERT_EQUAL_STRING("CHARGING", M().statusToString(ChargerStatus_t::CHARGING).c_str());
    TEST_ASSERT_EQUAL_STRING("SUSPENDED_EVSE", M().statusToString(ChargerStatus_t::SUSPENDED_EVSE).c_str());
    TEST_ASSERT_EQUAL_STRING("SUSPENDED_EV", M().statusToString(ChargerStatus_t::SUSPENDED_EV).c_str());
    TEST_ASSERT_EQUAL_STRING("FINISHING", M().statusToString(ChargerStatus_t::FINISHING).c_str());
    TEST_ASSERT_EQUAL_STRING("RESERVED", M().statusToString(ChargerStatus_t::RESERVED).c_str());
    TEST_ASSERT_EQUAL_STRING("UNAVAILABLE", M().statusToString(ChargerStatus_t::UNAVAILABLE).c_str());
    TEST_ASSERT_EQUAL_STRING("FAULTED", M().statusToString(ChargerStatus_t::FAULTED).c_str());
    TEST_ASSERT_EQUAL_STRING("INIT", M().statusToString(ChargerStatus_t::INIT).c_str());
    TEST_ASSERT_EQUAL_STRING("UNKNOWN", M().statusToString(static_cast<ChargerStatus_t>(99)).c_str());
}

static void test_mgr_onTimeEqual_second(void)
{
    mgr_reset();
    M().thread_ticks = 0;
    TEST_ASSERT_TRUE(M().onTimeEqual_Second(1U));
    M().thread_ticks = 4;
    TEST_ASSERT_FALSE(M().onTimeEqual_Second(1U));
}

static void test_mgr_findNext_AVAILABLE_to_PREPARING_when_cable_connected(void)
{
    mgr_reset();
    Hw().setCableStatus(HwCableStatus_t::CONNECTED);
    M().status = ChargerStatus_t::AVAILABLE;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ChargerStatus_t::PREPARING),
                          static_cast<int>(M().findNextChargerStatus(ChargerStatus_t::AVAILABLE)));
}

static void test_mgr_findNext_AVAILABLE_stays_when_cable_disconnected(void)
{
    mgr_reset();
    Hw().setCableStatus(HwCableStatus_t::NOT_CONNECTED);
    M().status = ChargerStatus_t::AVAILABLE;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ChargerStatus_t::AVAILABLE),
                          static_cast<int>(M().findNextChargerStatus(ChargerStatus_t::AVAILABLE)));
}

static void test_mgr_findNext_PREPARING_to_CHARGING(void)
{
    mgr_reset();
    Hw().setChargingForTest(true);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ChargerStatus_t::CHARGING),
                          static_cast<int>(M().findNextChargerStatus(ChargerStatus_t::PREPARING)));
}

static void test_mgr_findNext_PREPARING_stays(void)
{
    mgr_reset();
    Hw().setChargingForTest(false);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ChargerStatus_t::PREPARING),
                          static_cast<int>(M().findNextChargerStatus(ChargerStatus_t::PREPARING)));
}

static void test_mgr_findNext_CHARGING_group_cable_disconnect_finishing(void)
{
    mgr_reset();
    Hw().setCableStatus(HwCableStatus_t::NOT_CONNECTED);
    Hw().setChargingForTest(true);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ChargerStatus_t::FINISHING),
                          static_cast<int>(M().findNextChargerStatus(ChargerStatus_t::CHARGING)));
}

static void test_mgr_findNext_CHARGING_group_not_charging_finishing(void)
{
    mgr_reset();
    Hw().setCableStatus(HwCableStatus_t::CONNECTED);
    Hw().setChargingForTest(false);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ChargerStatus_t::FINISHING),
                          static_cast<int>(M().findNextChargerStatus(ChargerStatus_t::CHARGING)));
}

static void test_mgr_findNext_CHARGING_group_suspended_ev(void)
{
    mgr_reset();
    Hw().setCableStatus(HwCableStatus_t::CONNECTED);
    Hw().setChargingForTest(true);
    Hw().setEVDrawing(false);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ChargerStatus_t::SUSPENDED_EV),
                          static_cast<int>(M().findNextChargerStatus(ChargerStatus_t::CHARGING)));
}

static void test_mgr_findNext_CHARGING_group_stays_charging(void)
{
    mgr_reset();
    Hw().setCableStatus(HwCableStatus_t::CONNECTED);
    Hw().setChargingForTest(true);
    Hw().setEVDrawing(true);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ChargerStatus_t::CHARGING),
                          static_cast<int>(M().findNextChargerStatus(ChargerStatus_t::CHARGING)));
}

static void test_mgr_findNext_FINISHING_transitions_after_enough_ticks(void)
{
    mgr_reset();
    Hw().setFaultCode(0);
    for (int i = 0; i < 11; ++i) {
        TEST_ASSERT_EQUAL_INT(static_cast<int>(ChargerStatus_t::FINISHING),
                              static_cast<int>(M().findNextChargerStatus(ChargerStatus_t::FINISHING)));
    }
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ChargerStatus_t::AVAILABLE),
                          static_cast<int>(M().findNextChargerStatus(ChargerStatus_t::FINISHING)));
}

static void test_mgr_findNext_RESERVED_to_UNAVAILABLE(void)
{
    mgr_reset();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ChargerStatus_t::UNAVAILABLE),
                          static_cast<int>(M().findNextChargerStatus(ChargerStatus_t::RESERVED)));
}

static void test_mgr_findNext_UNAVAILABLE_stays(void)
{
    mgr_reset();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ChargerStatus_t::UNAVAILABLE),
                          static_cast<int>(M().findNextChargerStatus(ChargerStatus_t::UNAVAILABLE)));
}

static void test_mgr_findNext_FAULTED_clears_when_no_hw_fault(void)
{
    mgr_reset();
    Hw().setFaultCode(0);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ChargerStatus_t::AVAILABLE),
                          static_cast<int>(M().findNextChargerStatus(ChargerStatus_t::FAULTED)));
}

static void test_mgr_findNext_FAULTED_stays_when_hw_fault(void)
{
    mgr_reset();
    Hw().setFaultCode(1);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ChargerStatus_t::FAULTED),
                          static_cast<int>(M().findNextChargerStatus(ChargerStatus_t::FAULTED)));
}

static void test_mgr_findNext_default_INIT_to_UNAVAILABLE(void)
{
    mgr_reset();
    Hw().setFaultCode(0);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ChargerStatus_t::UNAVAILABLE),
                          static_cast<int>(M().findNextChargerStatus(ChargerStatus_t::INIT)));
}

static void test_mgr_findNext_any_overridden_by_hw_fault(void)
{
    mgr_reset();
    Hw().setCableStatus(HwCableStatus_t::NOT_CONNECTED);
    Hw().setFaultCode(3);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ChargerStatus_t::FAULTED),
                          static_cast<int>(M().findNextChargerStatus(ChargerStatus_t::AVAILABLE)));
}

static void test_mgr_chargerInitialConfigure(void)
{
    mgr_reset();
    M().chargerInitialConfigure();
    TEST_ASSERT_EQUAL_UINT8(1U, M().config.phaseAmount);
    TEST_ASSERT_EQUAL_INT(32000, M().config.currentLimit_HW);
    TEST_ASSERT_EQUAL_INT(6000, M().config.chargeCurrent_min);
    TEST_ASSERT_TRUE(M().running_);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ChargerStatus_t::AVAILABLE), static_cast<int>(M().status));
}

static void test_mgr_resetSessionData(void)
{
    mgr_reset();
    M().session.isActive_ = true;
    M().session.isAuthorized_ = true;
    M().session.authCardUid_ = "x";
    M().session.chargingData.energyDelivered_mWh = 99;
    M().resetSessionData();
    TEST_ASSERT_FALSE(M().session.isActive_);
    TEST_ASSERT_FALSE(M().session.isAuthorized_);
    TEST_ASSERT_EQUAL_UINT32(0U, M().session.chargingData.energyDelivered_mWh);
    TEST_ASSERT_EQUAL_STRING("", M().session.authCardUid_.c_str());
}

static void test_mgr_processDetectedCard(void)
{
    mgr_reset();
    M().processDetectedCard("ABC");
    TEST_ASSERT_EQUAL_STRING("ABC", M().detectedCard_.c_str());
}

static void test_mgr_startChargingSession_wrong_status(void)
{
    mgr_reset();
    M().status = ChargerStatus_t::AVAILABLE;
    M().startChargingSession_WithCard("04482B6A116280");
    TEST_ASSERT_EQUAL_STRING("", M().detectedCard_.c_str());
}

static void test_mgr_startChargingSession_in_preparing(void)
{
    mgr_reset();
    M().status = ChargerStatus_t::PREPARING;
    M().startChargingSession_WithCard("04482B6A116280");
    TEST_ASSERT_EQUAL_STRING("04482B6A116280", M().detectedCard_.c_str());
}

static void test_mgr_stopChargingSession_inactive(void)
{
    mgr_reset();
    M().session.isActive_ = false;
    M().stopChargingSession_WithCard("04482B6A116280");
    TEST_ASSERT_EQUAL_STRING("", M().detectedCard_.c_str());
}

static void test_mgr_stopChargingSession_active_charging(void)
{
    mgr_reset();
    M().session.isActive_ = true;
    M().status = ChargerStatus_t::CHARGING;
    M().stopChargingSession_WithCard("04482B6A116280");
    TEST_ASSERT_EQUAL_STRING("04482B6A116280", M().detectedCard_.c_str());
}

static void test_mgr_setChargingSessionCurrentLimit(void)
{
    mgr_reset();
    M().setChargingSessionCurrentLimit(16000);
    TEST_ASSERT_EQUAL_INT(16000, M().session.config.currentLimit);
    TEST_ASSERT_EQUAL_INT(16000, Hw().getLimit().current_mA);
}

static void test_mgr_setDetectedCard(void)
{
    mgr_reset();
    M().setDetectedCard("UID1");
    TEST_ASSERT_EQUAL_STRING("UID1", M().detectedCard_.c_str());
}

static void test_mgr_startAuthRequest_all_types(void)
{
    mgr_reset();
    M().config.auth.type = AuthType_t::AUTH_TYPE_NFC_CARD;
    M().startAuthRequest();
    TEST_ASSERT_TRUE(M().inAuthProcess_);

    mgr_reset();
    M().config.auth.type = AuthType_t::AUTH_TYPE_MATTER_CTRL;
    M().startAuthRequest();
    TEST_ASSERT_TRUE(M().inAuthProcess_);

    mgr_reset();
    M().config.auth.type = AuthType_t::AUTH_TYPE_NFC_W_TIME;
    M().startAuthRequest();
    TEST_ASSERT_EQUAL_INT(0, M().config.auth.timeCnt);

    mgr_reset();
    M().config.auth.type = AuthType_t::AUTH_TYPE_NONE;
    M().startAuthRequest();
    TEST_ASSERT_TRUE(M().inAuthProcess_);

    mgr_reset();
    M().config.auth.type = static_cast<AuthType_t>(999);
    M().startAuthRequest();
    TEST_ASSERT_TRUE(M().inAuthProcess_);
}

static void test_mgr_startAuthRequest_skips_when_already_in_progress(void)
{
    mgr_reset();
    M().inAuthProcess_ = true;
    M().startAuthRequest();
    TEST_ASSERT_TRUE(M().inAuthProcess_);
}

static void test_mgr_stopAuthRequest_all_types(void)
{
    mgr_reset();
    M().inAuthProcess_ = true;
    M().detectedCard_ = "card";
    M().config.auth.type = AuthType_t::AUTH_TYPE_NFC_CARD;
    M().stopAuthRequest();
    TEST_ASSERT_FALSE(M().inAuthProcess_);
    TEST_ASSERT_EQUAL_STRING("", M().detectedCard_.c_str());

    mgr_reset();
    M().inAuthProcess_ = true;
    M().config.auth.type = AuthType_t::AUTH_TYPE_MATTER_CTRL;
    M().stopAuthRequest();
    TEST_ASSERT_FALSE(M().inAuthProcess_);

    mgr_reset();
    M().inAuthProcess_ = true;
    M().detectedCard_ = "c2";
    M().config.auth.type = AuthType_t::AUTH_TYPE_NFC_W_TIME;
    M().stopAuthRequest();
    TEST_ASSERT_EQUAL_STRING("", M().detectedCard_.c_str());

    mgr_reset();
    M().inAuthProcess_ = true;
    M().config.auth.type = AuthType_t::AUTH_TYPE_NONE;
    M().stopAuthRequest();
    TEST_ASSERT_FALSE(M().inAuthProcess_);

    mgr_reset();
    M().inAuthProcess_ = true;
    M().config.auth.type = static_cast<AuthType_t>(888);
    M().stopAuthRequest();
    TEST_ASSERT_FALSE(M().inAuthProcess_);
}

static void test_mgr_stopAuthRequest_noop_when_not_in_auth(void)
{
    mgr_reset();
    M().inAuthProcess_ = false;
    M().stopAuthRequest();
    TEST_ASSERT_FALSE(M().inAuthProcess_);
}

static void test_mgr_checkIsAuthorized_routes(void)
{
    mgr_reset();
    M().status = ChargerStatus_t::PREPARING;
    M().config.auth.type = AuthType_t::AUTH_TYPE_MATTER_CTRL;
    TEST_ASSERT_TRUE(M().checkIsAuthorized());

    mgr_reset();
    M().status = ChargerStatus_t::CHARGING;
    M().config.auth.type = AuthType_t::AUTH_TYPE_MATTER_CTRL;
    TEST_ASSERT_FALSE(M().checkIsAuthorized());

    mgr_reset();
    M().status = ChargerStatus_t::AVAILABLE;
    TEST_ASSERT_FALSE(M().checkIsAuthorized());
}

static void test_mgr_checkAuthorizedPreparing_nfc_card(void)
{
    mgr_reset();
    M().status = ChargerStatus_t::PREPARING;
    M().config.auth.type = AuthType_t::AUTH_TYPE_NFC_CARD;
    TEST_ASSERT_FALSE(M().checkIsAuthorized());
    M().detectedCard_ = "04482B6A116280";
    TEST_ASSERT_TRUE(M().checkIsAuthorized());
    M().detectedCard_ = "wrong";
    TEST_ASSERT_FALSE(M().checkIsAuthorized());
}

static void test_mgr_checkAuthorizedPreparing_nfc_w_time_paths(void)
{
    mgr_reset();
    M().status = ChargerStatus_t::PREPARING;
    M().config.auth.type = AuthType_t::AUTH_TYPE_NFC_W_TIME;
    M().thread_ticks = 0;
    M().config.auth.timeCnt = 0;
    (void)M().checkIsAuthorized();

    mgr_reset();
    M().status = ChargerStatus_t::PREPARING;
    M().config.auth.type = AuthType_t::AUTH_TYPE_NFC_W_TIME;
    M().thread_ticks = 0;
    M().config.auth.timeCnt = 4;
    M().detectedCard_ = "04482B6A116280";
    TEST_ASSERT_TRUE(M().checkIsAuthorized());
}

static void test_mgr_checkAuthorizedActiveChargeState_nfc(void)
{
    mgr_reset();
    M().status = ChargerStatus_t::CHARGING;
    M().config.auth.type = AuthType_t::AUTH_TYPE_NFC_CARD;
    M().session.authCardUid_ = "A";
    M().detectedCard_ = "";
    TEST_ASSERT_FALSE(M().checkIsAuthorized());
    M().detectedCard_ = "A";
    TEST_ASSERT_TRUE(M().checkIsAuthorized());
    M().detectedCard_ = "B";
    TEST_ASSERT_FALSE(M().checkIsAuthorized());
}

static void test_mgr_checkHwFault(void)
{
    mgr_reset();
    Hw().setFaultCode(0);
    TEST_ASSERT_FALSE(M().checkHwFault());
    Hw().setFaultCode(2);
    TEST_ASSERT_TRUE(M().checkHwFault());
}

static void test_mgr_onSessionStart_and_end(void)
{
    mgr_reset();
    Hw().setLimit(HwChargingLimit_t{.current_mA = 31000});
    HwChargingMeter_t meter{};
    meter.energy_mWh = 50000;
    meter.current_mA = 8000;
    meter.voltage_mV = 230000;
    Hw().setMeter(meter);
    unit_test_esp_timer_now_us() = 5'000'000LL;
    M().onSessionStart();
    TEST_ASSERT_TRUE(M().session.isActive_);
    TEST_ASSERT_EQUAL_INT64(5'000'000LL, M().session.startTime);
    TEST_ASSERT_EQUAL_INT(31000, M().session.config.currentLimit);

    unit_test_esp_timer_now_us() = 7'000'000LL;
    M().onSessionEnd();
    TEST_ASSERT_FALSE(M().session.isActive_);
    TEST_ASSERT_EQUAL_INT64(2LL, M().session.timeElapsed);
}

static void test_mgr_findNextStatus_matter_gates_available(void)
{
    mgr_reset();
    M().status = ChargerStatus_t::AVAILABLE;
    MatterManager::GetInstance().SetChargingEnabledForTest(false);
    Hw().setCableStatus(HwCableStatus_t::NOT_CONNECTED);
    M().findNextStatus();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ChargerStatus_t::UNAVAILABLE), static_cast<int>(M().status));
}

static void test_mgr_findNextStatus_matter_gates_unavailable(void)
{
    mgr_reset();
    M().status = ChargerStatus_t::UNAVAILABLE;
    MatterManager::GetInstance().SetChargingEnabledForTest(true);
    M().findNextStatus();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ChargerStatus_t::AVAILABLE), static_cast<int>(M().status));
}

static void test_mgr_findNextStatus_on_hold_overrides(void)
{
    mgr_reset();
    M().status = ChargerStatus_t::AVAILABLE;
    Hw().setCableStatus(HwCableStatus_t::NOT_CONNECTED);
    MatterManager::GetInstance().SetChargingEnabledForTest(true);
    M().onHoldStatusTo(ChargerStatus_t::CHARGING);
    M().findNextStatus();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ChargerStatus_t::CHARGING), static_cast<int>(M().status));
}

static void test_mgr_showChargerDetails_no_crash(void)
{
    mgr_reset();
    M().showChargerDetails();
}

static void test_mgr_showChargingSessionInfo_phase_skip_and_show(void)
{
    mgr_reset();
    M().config.phaseAmount = 3;
    M().showChargingSessionInfo();

    mgr_reset();
    M().config.phaseAmount = 1;
    M().session.id = 7;
    M().session.startTime = 100;
    M().session.timeElapsed = 30;
    M().session.chargingData.energyDelivered_mWh = 5000;
    M().session.chargingData.current = 8000;
    M().session.chargingData.voltage = 230000;
    M().showChargingSessionInfo();
}

static void test_mgr_set_ready_and_power_board_no_crash(void)
{
    mgr_reset();
    M().setReadySemaphore(nullptr);
    M().setPowerBoardReady(true);
}

static void test_mgr_execCurrentStatus_available_break(void)
{
    mgr_reset();
    M().status = ChargerStatus_t::AVAILABLE;
    M().state_action_run_ = false;
    M().execCurrentStatus();
}

static void test_mgr_execCurrentStatus_preparing(void)
{
    mgr_reset();
    M().status = ChargerStatus_t::PREPARING;
    MatterManager::GetInstance().SetTargetsAllowForTest(true);
    M().config.auth.type = AuthType_t::AUTH_TYPE_NONE;
    M().execCurrentStatus();
}

static void test_mgr_execCurrentStatus_finishing(void)
{
    mgr_reset();
    M().status = ChargerStatus_t::FINISHING;
    M().state_action_run_ = false;
    M().session.startTime = 1'000'000LL;
    unit_test_esp_timer_now_us() = 3'000'000LL;
    HwChargingMeter_t m{};
    m.energy_mWh = 1000;
    m.current_mA = 5000;
    m.voltage_mV = 220000;
    Hw().setMeter(m);
    M().execCurrentStatus();
    TEST_ASSERT_TRUE(M().state_action_run_);
}

static void test_mgr_execCurrentStatus_faulted(void)
{
    mgr_reset();
    M().status = ChargerStatus_t::FAULTED;
    Hw().setFaultCode(5);
    M().execCurrentStatus();
}

static void test_mgr_runExecPreparingCase_early_when_state_action_run(void)
{
    mgr_reset();
    M().status = ChargerStatus_t::PREPARING;
    M().state_action_run_ = true;
    M().execCurrentStatus();
}

static void test_mgr_runExecPreparingCase_blocked_by_targets(void)
{
    mgr_reset();
    M().status = ChargerStatus_t::PREPARING;
    MatterManager::GetInstance().SetTargetsAllowForTest(false);
    M().config.auth.type = AuthType_t::AUTH_TYPE_NONE;
    M().execCurrentStatus();
}

static void test_mgr_runExecChargingLikeCase_meter_tick_and_apply_pause(void)
{
    mgr_reset();
    M().status = ChargerStatus_t::CHARGING;
    M().state_action_run_ = false;
    M().session.isActive_ = true;
    M().session.startTime = 1'000'000LL;
    unit_test_esp_timer_now_us() = 3'000'000LL;
    M().thread_ticks = 10;
    HwChargingMeter_t m{};
    m.energy_mWh = 7000;
    m.current_mA = 9000;
    m.voltage_mV = 235000;
    m.power_mW = 2000000;
    Hw().setMeter(m);
    M().config.auth.type = AuthType_t::AUTH_TYPE_NONE;
    MatterManager::GetInstance().SetTargetsAllowForTest(false);
    M().execCurrentStatus();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(HwChargeCommand_t::PAUSE), static_cast<int>(Hw().getChargeCommand()));
}

static void test_mgr_runExecChargingLikeCase_charging_disabled_stops(void)
{
    mgr_reset();
    M().status = ChargerStatus_t::CHARGING;
    M().state_action_run_ = false;
    M().session.isActive_ = true;
    M().session.startTime = 2'000'000LL;
    unit_test_esp_timer_now_us() = 4'000'000LL;
    M().thread_ticks = 10;
    HwChargingMeter_t m{};
    m.energy_mWh = 1;
    m.current_mA = 1;
    m.voltage_mV = 1;
    m.power_mW = 1;
    Hw().setMeter(m);
    M().config.auth.type = AuthType_t::AUTH_TYPE_NONE;
    MatterManager::GetInstance().SetTargetsAllowForTest(true);
    MatterManager::GetInstance().SetChargingEnabledForTest(false);
    M().execCurrentStatus();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(HwChargeCommand_t::STOP), static_cast<int>(Hw().getChargeCommand()));
}

static void test_mgr_apply_matter_suspended_evse_resume(void)
{
    mgr_reset();
    M().status = ChargerStatus_t::SUSPENDED_EVSE;
    M().state_action_run_ = false;
    M().session.isActive_ = true;
    M().session.startTime = 1'000'000LL;
    unit_test_esp_timer_now_us() = 2'000'000LL;
    M().thread_ticks = 10;
    HwChargingMeter_t m{};
    Hw().setMeter(m);
    MatterManager::GetInstance().SetTargetsAllowForTest(true);
    M().config.auth.type = AuthType_t::AUTH_TYPE_NONE;
    M().execCurrentStatus();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(HwChargeCommand_t::START), static_cast<int>(Hw().getChargeCommand()));
}

static void test_mgr_runExecChargingLikeCase_early_when_state_action_run(void)
{
    mgr_reset();
    M().status = ChargerStatus_t::CHARGING;
    M().state_action_run_ = true;
    M().execCurrentStatus();
}

void run_test_charger_manager_tests(void)
{
    RUN_TEST(test_mgr_statusToString_all_and_unknown);
    RUN_TEST(test_mgr_onTimeEqual_second);
    RUN_TEST(test_mgr_findNext_AVAILABLE_to_PREPARING_when_cable_connected);
    RUN_TEST(test_mgr_findNext_AVAILABLE_stays_when_cable_disconnected);
    RUN_TEST(test_mgr_findNext_PREPARING_to_CHARGING);
    RUN_TEST(test_mgr_findNext_PREPARING_stays);
    RUN_TEST(test_mgr_findNext_CHARGING_group_cable_disconnect_finishing);
    RUN_TEST(test_mgr_findNext_CHARGING_group_not_charging_finishing);
    RUN_TEST(test_mgr_findNext_CHARGING_group_suspended_ev);
    RUN_TEST(test_mgr_findNext_CHARGING_group_stays_charging);
    RUN_TEST(test_mgr_findNext_FINISHING_transitions_after_enough_ticks);
    RUN_TEST(test_mgr_findNext_RESERVED_to_UNAVAILABLE);
    RUN_TEST(test_mgr_findNext_UNAVAILABLE_stays);
    RUN_TEST(test_mgr_findNext_FAULTED_clears_when_no_hw_fault);
    RUN_TEST(test_mgr_findNext_FAULTED_stays_when_hw_fault);
    RUN_TEST(test_mgr_findNext_default_INIT_to_UNAVAILABLE);
    RUN_TEST(test_mgr_findNext_any_overridden_by_hw_fault);
    RUN_TEST(test_mgr_chargerInitialConfigure);
    RUN_TEST(test_mgr_resetSessionData);
    RUN_TEST(test_mgr_processDetectedCard);
    RUN_TEST(test_mgr_startChargingSession_wrong_status);
    RUN_TEST(test_mgr_startChargingSession_in_preparing);
    RUN_TEST(test_mgr_stopChargingSession_inactive);
    RUN_TEST(test_mgr_stopChargingSession_active_charging);
    RUN_TEST(test_mgr_setChargingSessionCurrentLimit);
    RUN_TEST(test_mgr_setDetectedCard);
    RUN_TEST(test_mgr_startAuthRequest_all_types);
    RUN_TEST(test_mgr_startAuthRequest_skips_when_already_in_progress);
    RUN_TEST(test_mgr_stopAuthRequest_all_types);
    RUN_TEST(test_mgr_stopAuthRequest_noop_when_not_in_auth);
    RUN_TEST(test_mgr_checkIsAuthorized_routes);
    RUN_TEST(test_mgr_checkAuthorizedPreparing_nfc_card);
    RUN_TEST(test_mgr_checkAuthorizedPreparing_nfc_w_time_paths);
    RUN_TEST(test_mgr_checkAuthorizedActiveChargeState_nfc);
    RUN_TEST(test_mgr_checkHwFault);
    RUN_TEST(test_mgr_onSessionStart_and_end);
    RUN_TEST(test_mgr_findNextStatus_matter_gates_available);
    RUN_TEST(test_mgr_findNextStatus_matter_gates_unavailable);
    RUN_TEST(test_mgr_findNextStatus_on_hold_overrides);
    RUN_TEST(test_mgr_showChargerDetails_no_crash);
    RUN_TEST(test_mgr_showChargingSessionInfo_phase_skip_and_show);
    RUN_TEST(test_mgr_set_ready_and_power_board_no_crash);
    RUN_TEST(test_mgr_execCurrentStatus_available_break);
    RUN_TEST(test_mgr_execCurrentStatus_preparing);
    RUN_TEST(test_mgr_execCurrentStatus_finishing);
    RUN_TEST(test_mgr_execCurrentStatus_faulted);
    RUN_TEST(test_mgr_runExecPreparingCase_early_when_state_action_run);
    RUN_TEST(test_mgr_runExecPreparingCase_blocked_by_targets);
    RUN_TEST(test_mgr_runExecChargingLikeCase_meter_tick_and_apply_pause);
    RUN_TEST(test_mgr_runExecChargingLikeCase_charging_disabled_stops);
    RUN_TEST(test_mgr_apply_matter_suspended_evse_resume);
    RUN_TEST(test_mgr_runExecChargingLikeCase_early_when_state_action_run);
}

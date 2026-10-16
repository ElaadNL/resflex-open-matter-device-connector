// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include "chargerManager.h"
#include "charger_uiux_handler.h"

#include "unity.h"

using CT::Charger::ChargerManager;
using CT::Charger::ChargerStatus_t;

static void mgr_reset()
{
    CT::Charger::ChargerManager::Controller().resetForTest();
}

static void test_uiux_exec_INIT_hits_default(void)
{
    mgr_reset();
    ChargerManager::Controller().status = ChargerStatus_t::INIT;
    CT::Charger::UIUX::execCurrentStatus();
}

static void test_uiux_exec_AVAILABLE(void)
{
    mgr_reset();
    ChargerManager::Controller().status = ChargerStatus_t::AVAILABLE;
    CT::Charger::UIUX::execCurrentStatus();
}

static void test_uiux_exec_PREPARING_unauthorized(void)
{
    mgr_reset();
    auto &m = ChargerManager::Controller();
    m.status = ChargerStatus_t::PREPARING;
    m.session.isAuthorized_ = false;
    CT::Charger::UIUX::execCurrentStatus();
}

static void test_uiux_exec_PREPARING_authorized(void)
{
    mgr_reset();
    auto &m = ChargerManager::Controller();
    m.status = ChargerStatus_t::PREPARING;
    m.session.isAuthorized_ = true;
    CT::Charger::UIUX::execCurrentStatus();
}

static void test_uiux_exec_CHARGING_active(void)
{
    mgr_reset();
    auto &m = ChargerManager::Controller();
    m.status = ChargerStatus_t::CHARGING;
    m.session.isActive_ = true;
    CT::Charger::UIUX::execCurrentStatus();
}

static void test_uiux_exec_CHARGING_inactive_routes_finishing(void)
{
    mgr_reset();
    auto &m = ChargerManager::Controller();
    m.status = ChargerStatus_t::CHARGING;
    m.session.isActive_ = false;
    m.thread_ticks = 10;
    m.session.timeElapsed = 10;
    m.session.chargingData.energyDelivered_mWh = 5000;
    CT::Charger::UIUX::execCurrentStatus();
}

static void test_uiux_exec_SUSPENDED_EVSE(void)
{
    mgr_reset();
    ChargerManager::Controller().status = ChargerStatus_t::SUSPENDED_EVSE;
    CT::Charger::UIUX::execCurrentStatus();
}

static void test_uiux_exec_SUSPENDED_EV(void)
{
    mgr_reset();
    ChargerManager::Controller().status = ChargerStatus_t::SUSPENDED_EV;
    CT::Charger::UIUX::execCurrentStatus();
}

static void test_uiux_exec_FINISHING(void)
{
    mgr_reset();
    auto &m = ChargerManager::Controller();
    m.status = ChargerStatus_t::FINISHING;
    m.thread_ticks = 10;
    m.session.timeElapsed = 99;
    m.session.chargingData.energyDelivered_mWh = 1234;
    CT::Charger::UIUX::execCurrentStatus();
}

static void test_uiux_exec_RESERVED_no_op(void)
{
    mgr_reset();
    ChargerManager::Controller().status = ChargerStatus_t::RESERVED;
    CT::Charger::UIUX::execCurrentStatus();
}

static void test_uiux_exec_UNAVAILABLE(void)
{
    mgr_reset();
    ChargerManager::Controller().status = ChargerStatus_t::UNAVAILABLE;
    CT::Charger::UIUX::execCurrentStatus();
}

static void test_uiux_exec_FAULTED(void)
{
    mgr_reset();
    ChargerManager::Controller().status = ChargerStatus_t::FAULTED;
    CT::Charger::UIUX::execCurrentStatus();
}

static void test_uiux_handle_charging_early_return_when_not_one_sec_tick(void)
{
    mgr_reset();
    auto &m = ChargerManager::Controller();
    m.thread_ticks = 4;
    m.uiux.itemIdx = 0;
    CT::Charger::UIUX::handleStatus_Charging();
}

static void test_uiux_handle_charging_itemIdx0_branch(void)
{
    mgr_reset();
    auto &m = ChargerManager::Controller();
    m.thread_ticks = 10;
    m.uiux.itemIdx = 0;
    m.session.chargingData.current = 10000;
    m.session.chargingData.voltage = 230;
    CT::Charger::UIUX::handleStatus_Charging();
}

static void test_uiux_handle_charging_itemIdx1_branch(void)
{
    mgr_reset();
    auto &m = ChargerManager::Controller();
    m.thread_ticks = 10;
    m.uiux.itemIdx = 1;
    m.session.timeElapsed = 60;
    m.session.chargingData.energyDelivered_mWh = 5000;
    m.session.chargingData.energyOffsetmWh = 1000;
    CT::Charger::UIUX::handleStatus_Charging();
}

static void test_uiux_handle_charging_itemIdx2_power_branch(void)
{
    mgr_reset();
    auto &m = ChargerManager::Controller();
    m.thread_ticks = 10;
    m.uiux.itemIdx = 2;
    m.session.chargingData.current = 16000;
    m.session.chargingData.voltage = 230;
    CT::Charger::UIUX::handleStatus_Charging();
}

static void test_uiux_handle_charging_sec5_tick_wraps_itemIdx(void)
{
    mgr_reset();
    auto &m = ChargerManager::Controller();
    m.thread_ticks = 25;
    m.uiux.itemIdx = 2;
    CT::Charger::UIUX::handleStatus_Charging();
    TEST_ASSERT_EQUAL_UINT32(0u, m.uiux.itemIdx);
}

static void test_uiux_handle_charging_sec5_advances_from_zero(void)
{
    mgr_reset();
    auto &m = ChargerManager::Controller();
    m.thread_ticks = 25;
    m.uiux.itemIdx = 0;
    CT::Charger::UIUX::handleStatus_Charging();
    TEST_ASSERT_EQUAL_UINT32(1u, m.uiux.itemIdx);
}

static void test_uiux_handle_finishing_early_return(void)
{
    mgr_reset();
    ChargerManager::Controller().thread_ticks = 4;
    CT::Charger::UIUX::handleStatus_Finishing();
}

static void test_uiux_handle_finishing_when_one_sec_tick(void)
{
    mgr_reset();
    auto &m = ChargerManager::Controller();
    m.thread_ticks = 10;
    m.session.timeElapsed = 42;
    m.session.chargingData.energyDelivered_mWh = 9999;
    CT::Charger::UIUX::handleStatus_Finishing();
}

static void test_uiux_handle_preparing_direct(void)
{
    mgr_reset();
    CT::Charger::UIUX::handleStatus_Preparing();
}

static void test_uiux_handle_suspended_evse_direct(void)
{
    mgr_reset();
    CT::Charger::UIUX::handleStatus_Suspended_EVSE();
}

static void test_uiux_handle_suspended_ev_direct(void)
{
    mgr_reset();
    CT::Charger::UIUX::handleStatus_Suspended_EV();
}

static void test_uiux_handle_unavailable_direct(void)
{
    mgr_reset();
    CT::Charger::UIUX::handleStatus_Unavailable();
}

static void test_uiux_handle_faulted_direct(void)
{
    mgr_reset();
    CT::Charger::UIUX::handleStatus_Faulted();
}

static void test_uiux_handle_available_direct(void)
{
    mgr_reset();
    CT::Charger::UIUX::handleStatus_Available();
}

void run_test_charger_uiux_handler_tests(void)
{
    RUN_TEST(test_uiux_exec_INIT_hits_default);
    RUN_TEST(test_uiux_exec_AVAILABLE);
    RUN_TEST(test_uiux_exec_PREPARING_unauthorized);
    RUN_TEST(test_uiux_exec_PREPARING_authorized);
    RUN_TEST(test_uiux_exec_CHARGING_active);
    RUN_TEST(test_uiux_exec_CHARGING_inactive_routes_finishing);
    RUN_TEST(test_uiux_exec_SUSPENDED_EVSE);
    RUN_TEST(test_uiux_exec_SUSPENDED_EV);
    RUN_TEST(test_uiux_exec_FINISHING);
    RUN_TEST(test_uiux_exec_RESERVED_no_op);
    RUN_TEST(test_uiux_exec_UNAVAILABLE);
    RUN_TEST(test_uiux_exec_FAULTED);
    RUN_TEST(test_uiux_handle_charging_early_return_when_not_one_sec_tick);
    RUN_TEST(test_uiux_handle_charging_itemIdx0_branch);
    RUN_TEST(test_uiux_handle_charging_itemIdx1_branch);
    RUN_TEST(test_uiux_handle_charging_itemIdx2_power_branch);
    RUN_TEST(test_uiux_handle_charging_sec5_tick_wraps_itemIdx);
    RUN_TEST(test_uiux_handle_charging_sec5_advances_from_zero);
    RUN_TEST(test_uiux_handle_finishing_early_return);
    RUN_TEST(test_uiux_handle_finishing_when_one_sec_tick);
    RUN_TEST(test_uiux_handle_preparing_direct);
    RUN_TEST(test_uiux_handle_suspended_evse_direct);
    RUN_TEST(test_uiux_handle_suspended_ev_direct);
    RUN_TEST(test_uiux_handle_unavailable_direct);
    RUN_TEST(test_uiux_handle_faulted_direct);
    RUN_TEST(test_uiux_handle_available_direct);
}

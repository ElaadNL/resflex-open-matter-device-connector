// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <random>
#include <map>
#include <ctime>
#include <cinttypes>

#ifndef UNIT_TEST
#include "esp_matter.h"
#else
#include "esp_timer.h"
#endif

#include "helpers.h"
#include "charger_uiux_handler.h"
#include "chargerManager.h"
#include "charger_session_math.h"

#ifndef UNIT_TEST
using namespace chip::app;
#endif
using namespace CT::Charger;

ChargerStatus_t ChargerManager::findNextChargerStatus(ChargerStatus_t current)
{
    ChargerStatus_t next = current;
    switch (current) {
    case ChargerStatus_t::AVAILABLE: {
        if (pHwControl->getCableStatus() == HwCableStatus_t::CONNECTED) {
            DEBUG_CHECKPOINT("EV is detected, checking authorization...");
            next = ChargerStatus_t::PREPARING;
        }
        break;
    }
    case ChargerStatus_t::PREPARING: {
        if (pHwControl->isCharging()) {
            next = ChargerStatus_t::CHARGING;
        } else {
            next = ChargerStatus_t::PREPARING;
        }
        break;
    }
    case ChargerStatus_t::CHARGING:
    case ChargerStatus_t::SUSPENDED_EVSE:
    case ChargerStatus_t::SUSPENDED_EV: {
        if (pHwControl->getCableStatus() != HwCableStatus_t::CONNECTED) {
            next = ChargerStatus_t::FINISHING;
            break;
        }
        if (!pHwControl->isCharging()) {
            next = ChargerStatus_t::FINISHING;
            break;
        }
        next = pHwControl->isEVDrawing() ? ChargerStatus_t::CHARGING : ChargerStatus_t::SUSPENDED_EV;
        break;
    }
    case ChargerStatus_t::FINISHING: {
        // introduce some delay to simulate the time for session to fully finish and update the energy delivered
        if (g_finishingTimeCnt200ms++ > 10) { // after 2 seconds, transition to AVAILABLE
            g_finishingTimeCnt200ms = 0;
            next = ChargerStatus_t::AVAILABLE;
        } else {
            next = ChargerStatus_t::FINISHING;
        }
        break;
    }
    case ChargerStatus_t::RESERVED: {
        next = ChargerStatus_t::UNAVAILABLE;
        break;
    }
    case ChargerStatus_t::UNAVAILABLE: {
        next = ChargerStatus_t::UNAVAILABLE;
        break;
    }
    case ChargerStatus_t::FAULTED: {
        if (!checkHwFault()) {
            // do I need to remember the previouse status
            // or I just switch to available
            next = ChargerStatus_t::AVAILABLE;
        } else {
            next = ChargerStatus_t::FAULTED;
        }
        break;
    }
    default: {
        next = ChargerStatus_t::UNAVAILABLE;
        break;
    }
    }

    if (checkHwFault()) {
        next = ChargerStatus_t::FAULTED;
    }

    return next;
}

void ChargerManager::setDetectedCard(const std::string &uid)
{
    // PRINTF_DEBUG("Detected NFC Card UID: %s", reader_utf8);
    ChargerManager::Controller().processDetectedCard(uid);
#ifndef UNIT_TEST
    ChargerManager::Controller().EE_dg->SendEvent_DetectedCard(
        chip::ByteSpan(reinterpret_cast<const uint8_t *>(uid.data()), uid.size()));
#endif

    PRINTF_DEBUG("Detected a card UID: %s", uid.c_str());
}

ChargerManager &ChargerManager::Controller()
{
    static ChargerManager singleton;
    return singleton;
}

// Destructor
ChargerManager::~ChargerManager()
{
    running_ = false; // Signal the thread to stop
#ifndef UNIT_TEST
    if (update_thread_.joinable()) {
        update_thread_.join(); // Wait for the thread to finish
    }
#endif
    PRINTF_DEBUG("ChargerManager shut down successfully.");
}

// --- Public Methods ---
void ChargerManager::showChargerDetails()
{
    DEBUG_CHECKPOINT("Charger Details:\n"
                     "\tPhase Amount: %" PRIu8 ""
                     "\tCurrent Limit\n"
                     "\t\tHW:\t\t%.1f A\n"
                     "\t\tSetting:\t%.1f A",
                     config.phaseAmount, static_cast<double>(config.currentLimit_HW) / 1000.0,
                     static_cast<double>(session.config.currentLimit) / 1000.0);
}

void ChargerManager::showChargingSessionInfo()
{
    // for now just show the single phase data
    if (config.phaseAmount != 1)
        return;

    // show current charging session info
    //  startTime is esp_timer_get_time() (int64, microseconds since boot). Do not use %lu — on
    //  32-bit targets that mismatches int64_t in varargs and breaks all following %f specifiers.
    DEBUG_CHECKPOINT("Charging Session Info:\n"
                     "\tSession ID:\t\t%" PRIu32 "\n"
                     "\tStart (monotonic us):\t%" PRId64 "\n"
                     "\tTime Elapsed:\t\t%.2f sec\n"
                     "\tEnergy Delivered:\t%.2f Wh\n"
                     "\tCurrent:\t\t%.1f A\n"
                     "\tVoltage:\t\t%.1f V",
                     session.id, session.startTime, static_cast<double>(session.timeElapsed),
                     static_cast<double>(session.chargingData.energyDelivered_mWh) / 1000.0,
                     static_cast<double>(session.chargingData.current) / 1000.0,
                     static_cast<double>(session.chargingData.voltage) / 1000.0);
}

void ChargerManager::processDetectedCard(const std::string &uid)
{
    detectedCard_ = uid;
    PRINTF_DEBUG("Detected a card UID: %s", detectedCard_.c_str());
}

void ChargerManager::startChargingSession_WithCard(const std::string &uid)
{
    if (status != ChargerStatus_t::PREPARING) {
        // cannot start charging session if not in preparing state
        PRINTF_DEBUG("Cannot start charging session. Charger not in PREPARING state.");
        return;
    } else {
        processDetectedCard(uid);
    }
}

void ChargerManager::stopChargingSession_WithCard(const std::string &uid)
{
    if (!session.isActive_)
        return;

    if (status == ChargerStatus_t::CHARGING || status == ChargerStatus_t::SUSPENDED_EVSE ||
        status == ChargerStatus_t::SUSPENDED_EV) {
        processDetectedCard(uid);
    }
}

void ChargerManager::setChargingSessionCurrentLimit(int currentLimit)
{
    session.config.currentLimit = currentLimit;
    pHwControl->setLimit(HwChargingLimit_t{.current_mA = currentLimit});

    DEBUG_CHECKPOINT("Charging session current limit set to %" PRId32 " mA", static_cast<int32_t>(currentLimit));
}

void ChargerManager::resetSessionData()
{
    session.isActive_ = false;
    session.isAuthorized_ = false;

    session.startTime = 0;
    session.stopTime = 0;
    session.chargingData.energyDelivered_mWh = 0;
    session.chargingData.energyOffsetmWh = 0;

    config.auth.inProgress = false;
    session.authCardUid_.clear();

    // session.config.currentLimit = 0;
    PRINTF_DEBUG("Reset Session Data");
}

void ChargerManager::startAuthRequest()
{
    if (inAuthProcess_)
        return;
    inAuthProcess_ = true;

    if (config.auth.type == AuthType_t::AUTH_TYPE_NFC_CARD) {
        PRINTF_DEBUG("Auth Type: NFC");
    } else if (config.auth.type == AuthType_t::AUTH_TYPE_MATTER_CTRL) {
        PRINTF_DEBUG("Auth Type: Matter Control");
    } else if (config.auth.type == AuthType_t::AUTH_TYPE_NFC_W_TIME) {
        PRINTF_DEBUG("Auth Type: NFC with TimeBased");
        config.auth.timeCnt = 0;
    } else if (config.auth.type == AuthType_t::AUTH_TYPE_NONE) {
        PRINTF_DEBUG("No Authorization");
    } else {
        PRINTF_DEBUG("Authorization type not supported");
    }
}

void ChargerManager::stopAuthRequest()
{
    if (inAuthProcess_ == false)
        return;
    inAuthProcess_ = false;

    if (config.auth.type == AuthType_t::AUTH_TYPE_NFC_CARD) {
        PRINTF_DEBUG("Auth Type: NFC");

        detectedCard_.clear();
    } else if (config.auth.type == AuthType_t::AUTH_TYPE_MATTER_CTRL) {
        PRINTF_DEBUG("Auth Type: Matter Control");
    } else if (config.auth.type == AuthType_t::AUTH_TYPE_NFC_W_TIME) {
        PRINTF_DEBUG("Auth Type: NFC with TimeBased");

        detectedCard_.clear();
    } else if (config.auth.type == AuthType_t::AUTH_TYPE_NONE) {
        PRINTF_DEBUG("No Authorization");
    } else {
        PRINTF_DEBUG("Authorization type not supported");
    }
}

bool ChargerManager::checkIsAuthorized()
{
    if (status == ChargerStatus_t::PREPARING) {
        return checkAuthorizedPreparingState();
    }
    if (status == ChargerStatus_t::CHARGING || status == ChargerStatus_t::SUSPENDED_EVSE ||
        status == ChargerStatus_t::SUSPENDED_EV) {
        return checkAuthorizedActiveChargeState();
    }
    return false;
}

bool ChargerManager::checkAuthorizedPreparingState()
{
    switch (config.auth.type) {
    case AuthType_t::AUTH_TYPE_NFC_CARD:
        if (detectedCard_.empty()) {
            return false;
        }
        if (detectedCard_ == "04482B6A116280") // sample authorized card uid
        {
            session.authCardUid_ = detectedCard_;
            return true;
        }
        PRINTF_DEBUG("Charging session cannot start for card UID: %s", detectedCard_.c_str());
        return false;
    case AuthType_t::AUTH_TYPE_MATTER_CTRL:
        PRINTF_DEBUG("Matter Control authorization granted.");
        return true;
    case AuthType_t::AUTH_TYPE_NFC_W_TIME:
        if (onTimeEqual_Second(1)) {
            config.auth.timeCnt++;
            PRINTF_DEBUG("TimeBased: %d", config.auth.timeCnt);
        }
        if (config.auth.timeCnt >= 5) {
            PRINTF_DEBUG("Times Up, Auto start charging session!");
            return true;
        }
        if (detectedCard_.empty()) {
            return false;
        }
        if (detectedCard_ == "04482B6A116280") // sample authorized card uid
        {
            session.authCardUid_ = detectedCard_;
            return true;
        }
        PRINTF_DEBUG("Charging session cannot start for card UID: %s", detectedCard_.c_str());
        return false;
    default:
        return false;
    }
}

bool ChargerManager::checkAuthorizedActiveChargeState()
{
    switch (config.auth.type) {
    case AuthType_t::AUTH_TYPE_NFC_CARD:
    case AuthType_t::AUTH_TYPE_NFC_W_TIME:
        if (detectedCard_.empty()) {
            return false;
        }
        if (detectedCard_ == session.authCardUid_) {
            PRINTF_DEBUG("Stopping charging session as requested by %s", detectedCard_.c_str());
            return true;
        }
        PRINTF_DEBUG("Card UID mismatch. Cannot stop charging session for card UID: %s", detectedCard_.c_str());
        return false;
    case AuthType_t::AUTH_TYPE_MATTER_CTRL:
        // Matter doesnt have direct control to stop a session — unplug stops session
        return false;
    default:
        return false;
    }
}

void ChargerManager::onSessionStart()
{
    session.isActive_ = true;

    session.id++;

    // get target charging current
    session.config.currentLimit = pHwControl->getLimit().current_mA;

    // Record session start time
    session.timeElapsed = 0;
    session.startTime = esp_timer_get_time();
    session.chargingData.energyDelivered_mWh = 0;

    session.chargingData.energyOffsetmWh = 0;

    MatterManager::GetInstance().StartSession(session.chargingData.energyOffsetmWh);
}

void ChargerManager::onSessionEnd()
{
    session.isActive_ = false;

    // Record session end time
    session.stopTime = esp_timer_get_time();
    session.timeElapsed = session_time_elapsed_sec_from_monotonic_us(session.stopTime, session.startTime);

    int64_t _energy = pHwControl->getMeter().energy_mWh;
    int32_t _current = pHwControl->getMeter().current_mA;
    int32_t _voltage = pHwControl->getMeter().voltage_mV;

    session.chargingData.energyDelivered_mWh =
        session_energy_delivered_mWh(_energy, static_cast<int64_t>(session.chargingData.energyOffsetmWh));
    MatterManager::GetInstance().StopSession(_energy);

    // print session summary
    DEBUG_CHECKPOINT("Charging session ended.\n"
                     "\tDuration: %.2f sec\n"
                     "\tEnergy Delivered: %.2f Wh\n"
                     "\tLast Meter Data:\n"
                     "\t\tCurrent L1: %.1f A\n"
                     "\t\tVoltage L1: %.1f V\n",
                     static_cast<double>(session.timeElapsed),
                     static_cast<double>(session.chargingData.energyDelivered_mWh) / 1000.0,
                     static_cast<double>(_current) / 1000.0, static_cast<double>(_voltage) / 1000.0);
}

bool ChargerManager::checkHwFault()
{
#ifndef UNIT_TEST
    chip::app::Clusters::EnergyEvse::FaultStateEnum f =
        static_cast<chip::app::Clusters::EnergyEvse::FaultStateEnum>(pHwControl->getFaultCode());
    if (f != chip::app::Clusters::EnergyEvse::FaultStateEnum::kNoError) {
        PRINTF_DEBUG("Fault detected! Fault code: 0x%02X", static_cast<uint8_t>(f));
        return true;
    }
    return false;
#else
    const uint8_t code = pHwControl->getFaultCode();
    if (code != 0U) {
        PRINTF_DEBUG("Fault detected! Fault code: 0x%02X", code);
        return true;
    }
    return false;
#endif
}

void ChargerManager::findNextStatus()
{
    ChargerStatus_t curr_status = status;
    ChargerStatus_t next_status = findNextChargerStatus(status);

    // special handling for AVAILABLE and UNAVAILABLE based on Matter charging enabled state
    if (curr_status == ChargerStatus_t::AVAILABLE) {
        if (MatterManager::GetInstance().GetChargingEnabled() == false) {
            next_status = ChargerStatus_t::UNAVAILABLE;
        }
    } else if (curr_status == ChargerStatus_t::UNAVAILABLE) {
        if (MatterManager::GetInstance().GetChargingEnabled()) {
            next_status = ChargerStatus_t::AVAILABLE;
        }
    }

    if (onHoldStatus_ != ChargerStatus_t::INIT && next_status != onHoldStatus_) {
        next_status = onHoldStatus_;
    }

    if (next_status != curr_status) {
        DEBUG_CHECKPOINT("Charger status changed:\n" LOG_COLOR(LOG_COLOR_YELLOW) "[ %s ] >>> [ %s ]",
                         statusToString(curr_status).c_str(), statusToString(next_status).c_str());

        // handle the privous status
        if (curr_status == ChargerStatus_t::FINISHING) {
            resetSessionData();
        }

        // reset temporarily flag and data
        state_action_run_ = false;
        thread_ticks = 0;
        uiux.itemIdx = 0;

        // stop auth process
        stopAuthRequest();

        // set current status to next
        status = next_status;

        // notify matter for status changed
        MatterManager::GetInstance().UpdateState();

        CT::Charger::UIUX::execCurrentStatus();
    }
}

void ChargerManager::runExecPreparingCase()
{
    if (state_action_run_) {
        return;
    }

    if (!inAuthProcess_) {
        startAuthRequest();
    }

    bool canStart = true;
    if (config.auth.type != AuthType_t::AUTH_TYPE_NONE) {
        canStart = checkIsAuthorized();
    }
    if (!canStart || !MatterManager::GetInstance().IsChargingAllowedByTargets()) {
        return;
    }

    state_action_run_ = true;
    session.isAuthorized_ = true;
    pHwControl->setChargeCommand(HwChargeCommand_t::START);
}

void ChargerManager::applyMatterTargetChargeControls()
{
    if (status == ChargerStatus_t::CHARGING || status == ChargerStatus_t::SUSPENDED_EV) {
        if (!MatterManager::GetInstance().IsChargingAllowedByTargets()) {
            pHwControl->setChargeCommand(HwChargeCommand_t::PAUSE);
            return;
        }
        if (!MatterManager::GetInstance().GetChargingEnabled()) {
            state_action_run_ = true;
            session.isActive_ = false;
            pHwControl->setChargeCommand(HwChargeCommand_t::STOP);
        }
        return;
    }

    if (status != ChargerStatus_t::SUSPENDED_EVSE) {
        return;
    }
    if (MatterManager::GetInstance().IsChargingAllowedByTargets()) {
        pHwControl->setChargeCommand(HwChargeCommand_t::START);
    }
}

void ChargerManager::runExecChargingLikeCase()
{
    if (state_action_run_) {
        return;
    }

    if (!session.isActive_) {
        onSessionStart();
    }

    session.timeElapsed = session_time_elapsed_sec_from_monotonic_us(esp_timer_get_time(), session.startTime);
    PRINTF_DEBUG("Time Elapsed: %.2f sec", session.timeElapsed * 1.0);

    if (onTimeEqual_Second(1)) {
        HwChargingMeter_t meter = pHwControl->getMeter();

        session.chargingData.energyDelivered_mWh = meter.energy_mWh;
        session.chargingData.current = meter.current_mA;
        session.chargingData.voltage = meter.voltage_mV;

        MatterManager::GetInstance().UpdateSession(meter.energy_mWh);
        MatterManager::GetInstance().SendReadings(meter.power_mW, meter.voltage_mV, meter.current_mA);
        MatterManager::GetInstance().SendCumulativeEnergyReading(meter.energy_mWh, 0);

        showChargingSessionInfo();
        applyMatterTargetChargeControls();
    }

    if (!inAuthProcess_) {
        startAuthRequest();
    }

    bool canStop = false;
    if (config.auth.type != AuthType_t::AUTH_TYPE_NONE) {
        canStop = checkIsAuthorized();
    }
    if (!canStop) {
        return;
    }

    state_action_run_ = true;
    session.isActive_ = false;
    pHwControl->setChargeCommand(HwChargeCommand_t::STOP);
}

void ChargerManager::execCurrentStatus()
{
    switch (status) {
    case ChargerStatus_t::AVAILABLE: {
        if (state_action_run_)
            break;
    } break;
    case ChargerStatus_t::PREPARING:
        runExecPreparingCase();
        break;
    case ChargerStatus_t::CHARGING:
    case ChargerStatus_t::SUSPENDED_EVSE:
    case ChargerStatus_t::SUSPENDED_EV:
        runExecChargingLikeCase();
        break;
    case ChargerStatus_t::FINISHING: {
        if (state_action_run_)
            break;
        state_action_run_ = true;

        pHwControl->setChargeCommand(HwChargeCommand_t::STOP);

        onSessionEnd();

        pHwControl->resetMeterData();
    } break;
    case ChargerStatus_t::RESERVED:
        // Logic to handle RESERVED state
        break;
    case ChargerStatus_t::UNAVAILABLE:
        // Logic to handle UNAVAILABLE state
        break;
    case ChargerStatus_t::FAULTED: {
        MatterManager::GetInstance().UpdateFaultState(pHwControl->getFaultCode());
        pHwControl->setChargeCommand(HwChargeCommand_t::STOP);
    } break;
    default:
        break;
    }
}

void ChargerManager::onHoldStatusTo(ChargerStatus_t status)
{
    onHoldStatus_ = status;
}

void ChargerManager::mainLoop()
{
    chargerInitialConfigure();

    // using namespace std::chrono;
    // auto next_wake_time = steady_clock::now();

    while (running_) {
        findNextStatus();
        execCurrentStatus();

        // Sleep for a short duration before the next update
        std::this_thread::sleep_for(std::chrono::milliseconds(CHARGER_MGR_THREAD_TICKS));
        thread_ticks++;
    }
}

void ChargerManager::chargerInitialConfigure()
{
    config.phaseAmount = 1;
    config.currentLimit_HW = 32000;

    config.chargeCurrent_min = 6000; // 6000 mA is the minimum charging current;
    config.chargeCurrent_max = config.currentLimit_HW; // depends on Hardware combinations

    session.config.currentLimit = config.currentLimit_HW;

    status = ChargerStatus_t::AVAILABLE;

    running_ = true;
}

std::string ChargerManager::statusToString(ChargerStatus_t status) const
{
    switch (status) {
    case ChargerStatus_t::AVAILABLE:
        return "AVAILABLE";
    case ChargerStatus_t::PREPARING:
        return "PREPARING";
    case ChargerStatus_t::CHARGING:
        return "CHARGING";
    case ChargerStatus_t::SUSPENDED_EVSE:
        return "SUSPENDED_EVSE";
    case ChargerStatus_t::SUSPENDED_EV:
        return "SUSPENDED_EV";
    case ChargerStatus_t::FINISHING:
        return "FINISHING";
    case ChargerStatus_t::RESERVED:
        return "RESERVED";
    case ChargerStatus_t::UNAVAILABLE:
        return "UNAVAILABLE";
    case ChargerStatus_t::FAULTED:
        return "FAULTED";
    case ChargerStatus_t::INIT:
        return "INIT";
    default:
        return "UNKNOWN";
    }
}

bool ChargerManager::onTimeEqual_Second(uint32_t sec)
{
    return charger_thread_tick_aligns_interval(thread_ticks, sec, CHARGER_MGR_THREAD_TICKS);
}

void ChargerManager::setReadySemaphore(SemaphoreHandle_t semaphore)
{
    (void)semaphore;
}

void ChargerManager::setPowerBoardReady(bool ready)
{
    (void)ready;
}

#ifdef UNIT_TEST
void ChargerManager::resetForTest()
{
    g_finishingTimeCnt200ms = 0;
    unit_test_esp_timer_now_us() = 1'000'000LL;

    HardwareControlInterface::Instance().resetForTest();
    MatterManager::GetInstance().resetForTest();

    running_ = false;
    status = ChargerStatus_t::INIT;
    onHoldStatus_ = ChargerStatus_t::INIT;
    thread_ticks = 0;
    uiux.itemIdx = 0;
    state_action_run_ = false;
    inAuthProcess_ = false;
    detectedCard_.clear();
    EE_dg = nullptr;

    config.phaseAmount = 1;
    config.currentLimit_HW = 32000;
    config.chargeCurrent_min = 6000;
    config.chargeCurrent_max = config.currentLimit_HW;
    config.auth.type = AuthType_t::AUTH_TYPE_NONE;
    config.auth.inProgress = false;
    config.auth.timeCnt = 0;

    resetSessionData();
    session.config.currentLimit = config.currentLimit_HW;
}
#endif

ChargerManager::ChargerManager()
{
    running_ = false;
    config.auth.type = AuthType_t::AUTH_TYPE_NONE;
    status = ChargerStatus_t::INIT;
    pHwControl = &CT::Charger::HardwareControlInterface::Instance();

    resetSessionData();

#ifndef UNIT_TEST
    update_thread_ = std::thread(&ChargerManager::mainLoop, this);
#endif
    PRINTF_DEBUG("ChargerManager created.");
}
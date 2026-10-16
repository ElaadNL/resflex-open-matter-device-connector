// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

#include "helpers.h"

#ifndef UNIT_TEST
#include "hardwareControlInterface.h"
#include "matterManager.h"
#else
#include "hardwareControlInterface.h"
#include "matterManager_unit_stub.h"
typedef void *SemaphoreHandle_t;
#endif

#define CHARGER_MGR_THREAD_TICKS (200) // 200ms for each run

namespace CT {
namespace Charger {

class ChargerManager {
public:
    static ChargerManager &Controller();

    ChargerManager(ChargerManager const &) = delete;
    void operator=(ChargerManager const &) = delete;

    ~ChargerManager();

    void SetMatterDelegateEnergyEvse(chip::app::Clusters::EnergyEvse::EnergyEvseDelegate *delegate)
    {
        EE_dg = delegate;
    }

#ifdef UNIT_TEST
    chip::app::Clusters::EnergyEvse::EnergyEvseDelegate *EE_dg = nullptr;
#else
    chip::app::Clusters::EnergyEvse::EnergyEvseDelegate *EE_dg;
#endif

    void showChargerDetails();
    void showChargingSessionInfo();

    void onHoldStatusTo(ChargerStatus_t status);

    void setReadySemaphore(SemaphoreHandle_t semaphore);
    void setPowerBoardReady(bool ready);
    void processDetectedCard(const std::string &uid);

    void startChargingSession_WithCard(const std::string &uid);
    void stopChargingSession_WithCard(const std::string &uid);
    void setChargingSessionCurrentLimit(int currentLimit);

    std::thread update_thread_;
    std::atomic<bool> running_;
    std::atomic<ChargerStatus_t> onHoldStatus_{ChargerStatus_t::INIT};

    struct {
        std::atomic<bool> isActive_;
        std::atomic<bool> isAuthorized_;

        std::string authCardUid_;

        uint32_t id;

        int64_t startTime;
        int64_t stopTime;
        int64_t timeElapsed;

        struct {
            uint32_t energyDelivered_mWh;
            uint32_t energyOffsetmWh;
            uint32_t current;
            uint32_t voltage;
        } chargingData;

        struct {
            int currentLimit;
        } config;
    } session;

    struct {
        uint8_t phaseAmount;
        int currentLimit_HW;
        int chargeCurrent_min;
        int chargeCurrent_max;

        struct {
            std::atomic<bool> inProgress;
            AuthType_t type;
            int timeCnt;
        } auth;
    } config;

    // for display and LED
    struct {
        uint32_t itemIdx = 0;
    } uiux;

    uint32_t thread_ticks = 0;
    uint32_t g_finishingTimeCnt200ms = 0;

    std::atomic<bool> state_action_run_;
    std::atomic<bool> inAuthProcess_;

    std::string detectedCard_;
    void setDetectedCard(const std::string &uid);

    HardwareControlInterface *pHwControl;

    ChargerStatus_t status;
    ChargerStatus_t findNextChargerStatus(ChargerStatus_t current);

    void resetSessionData();
    void startAuthRequest();
    void stopAuthRequest();
    bool checkIsAuthorized();

    void onSessionStart();
    void onSessionEnd();

    bool checkHwFault();
    void findNextStatus();
    void execCurrentStatus();

    void mainLoop();
    void chargerInitialConfigure();

    std::string statusToString(ChargerStatus_t status) const;

    bool onTimeEqual_Second(uint32_t sec);

#ifdef UNIT_TEST
    /// Reset singleton state between host tests (no thread / no Matter stack).
    void resetForTest();
#endif

private:
    bool checkAuthorizedPreparingState();
    bool checkAuthorizedActiveChargeState();
    void runExecPreparingCase();
    void runExecChargingLikeCase();
    void applyMatterTargetChargeControls();

    ChargerManager();
};

} // namespace Charger
} // namespace CT
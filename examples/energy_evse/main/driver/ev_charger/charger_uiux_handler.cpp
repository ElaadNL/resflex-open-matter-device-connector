// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include "helpers.h"
#include "chargerManager.h"
#include "charger_uiux_handler.h"

namespace CT {
namespace Charger {

namespace UIUX {

void execCurrentStatus()
{
    ChargerStatus_t st = ChargerManager::Controller().status;
    switch (st) {
    case ChargerStatus_t::AVAILABLE: {
        handleStatus_Available();
        break;
    }
    case ChargerStatus_t::PREPARING: {
        if (ChargerManager::Controller().session.isAuthorized_)
            handleStatus_Suspended_EV();
        else
            handleStatus_Preparing();
        break;
    }
    case ChargerStatus_t::CHARGING: {
        if (ChargerManager::Controller().session.isActive_)
            handleStatus_Charging();
        else
            handleStatus_Finishing();
        break;
    }
    case ChargerStatus_t::SUSPENDED_EVSE: {
        handleStatus_Suspended_EVSE();
        break;
    }
    case ChargerStatus_t::SUSPENDED_EV: {
        handleStatus_Suspended_EV();
        break;
    }
    case ChargerStatus_t::FINISHING: {
        handleStatus_Finishing();
        break;
    }
    case ChargerStatus_t::RESERVED: {
        break;
    }
    case ChargerStatus_t::UNAVAILABLE: {
        handleStatus_Unavailable();
        break;
    }
    case ChargerStatus_t::FAULTED: {
        handleStatus_Faulted();
        break;
    }
    default:
        break;
    }
}

void handleStatus_Available()
{
#ifndef UNIT_TEST
    PRINTF_DEBUG("Matter[%s], EV is available",
                 MatterManager::GetInstance().isConnected ? "Connected" : "Not Connected");
#else
    PRINTF_DEBUG("EV is available (UNIT_TEST)");
#endif
}

void handleStatus_Preparing()
{
    PRINTF_DEBUG("EV is detected, waiting for authorization...");
}

void handleStatus_Charging()
{
    ChargerManager *mgr = &ChargerManager::Controller();
    if (ChargerManager::Controller().onTimeEqual_Second(5)) {
        mgr->uiux.itemIdx++;
        mgr->uiux.itemIdx = (mgr->uiux.itemIdx % 3);
    }

    if (!ChargerManager::Controller().onTimeEqual_Second(1))
        return;
    if (mgr->uiux.itemIdx == 0) {
        uint32_t current = mgr->session.chargingData.current;
        uint32_t voltage = mgr->session.chargingData.voltage;

        PRINTF_DEBUG("Current: %u mA, Voltage: %u V", current, voltage);
    } else if (mgr->uiux.itemIdx == 1) {
        uint32_t timeDiff = mgr->session.timeElapsed;
        uint64_t currentEnergy = mgr->session.chargingData.energyDelivered_mWh;
        uint64_t startEnergy = mgr->session.chargingData.energyOffsetmWh;
        uint32_t energyDiff = (uint32_t)(currentEnergy - startEnergy);

        PRINTF_DEBUG("Time Elapsed: %u sec, Energy Delivered: %u Wh", timeDiff, energyDiff);
    } else {
        uint32_t power = mgr->session.chargingData.current * mgr->session.chargingData.voltage / 1000; // in mW
        PRINTF_DEBUG("Power: %u kW", power);
    }
}

void handleStatus_Suspended_EVSE()
{
    PRINTF_DEBUG("Suspended by EVSE, show suspended message on UI");
}

void handleStatus_Suspended_EV()
{
    PRINTF_DEBUG("Suspended by EV, show suspended message on UI");
}

void handleStatus_Finishing()
{
    ChargerManager *mgr = &ChargerManager::Controller();
    if (!ChargerManager::Controller().onTimeEqual_Second(1))
        return;

    uint32_t timeDiff = mgr->session.timeElapsed;
    uint32_t energyEnd = (uint32_t)(mgr->session.chargingData.energyDelivered_mWh);

    PRINTF_DEBUG("Finishing - Time Diff: %u sec, Energy Delivered: %u Wh", timeDiff, energyEnd);
}

void handleStatus_Unavailable()
{
    PRINTF_DEBUG("Charger is in UNAVAILABLE state, show unavailable message on UI");
}

void handleStatus_Faulted()
{
    PRINTF_DEBUG("Charger is in FAULTED state, show error code on UI");
}

} // namespace UIUX

} // namespace Charger
} // namespace CT
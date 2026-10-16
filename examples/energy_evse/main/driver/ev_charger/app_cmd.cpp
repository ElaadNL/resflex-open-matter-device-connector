// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include <cstring>
#include <cinttypes>
#include <cstdint>
#include <cstdlib>
#include <iterator>

#ifndef UNIT_TEST
#include <esp_check.h>
#include <esp_log.h>
#include "esp_console.h"
#include <esp_matter_console.h>
#include <esp_matter_core.h>

#include "hardwareControlInterface.h"
#include "chargerManager.h"
#endif

#include "helpers.h"
#include "app_cmd.h"

namespace {
[[maybe_unused]] constexpr const char kPromptStr[] = "EVC";
} // namespace

const char *const TAG = "charger general command";

void hw_exec_cable(int argc, char **argv)
{
    if (argc <= 2) {
        return;
    }
    auto const connected = strtol(argv[2], nullptr, 10) != 0;
#ifndef UNIT_TEST
    CT::Charger::HardwareControlInterface::Instance().setCableStatus(
        connected ? CT::Charger::HwCableStatus_t::CONNECTED : CT::Charger::HwCableStatus_t::NOT_CONNECTED);
#endif
}

void hw_exec_limit(int argc, char **argv)
{
    if (argc <= 2) {
        return;
    }
    auto const limit = strtoll(argv[2], nullptr, 10);
#ifndef UNIT_TEST
    CT::Charger::ChargerManager::Controller().setChargingSessionCurrentLimit(static_cast<int>(limit));
    DEBUG_CHECKPOINT("emulator: charge limit set to %" PRId64 " mA", static_cast<int64_t>(limit));
#endif
}

void hw_exec_ev(int argc, char **argv)
{
    if (argc <= 2) {
        return;
    }
    auto const isDrawing = strtol(argv[2], nullptr, 10) != 0;
#ifndef UNIT_TEST
    CT::Charger::HardwareControlInterface::Instance().setEVDrawing(isDrawing);
    DEBUG_CHECKPOINT("emulator: set EV drawing to %s", isDrawing ? "true" : "false");
#endif
}

void hw_exec_fault(int argc, char **argv)
{
    if (argc <= 2) {
        return;
    }
    auto const faultCode = static_cast<uint8_t>(strtoul(argv[2], nullptr, 10));
#ifndef UNIT_TEST
    CT::Charger::HardwareControlInterface::Instance().setFaultCode(faultCode);
    DEBUG_CHECKPOINT("emulator: set fault code to %" PRIu8, faultCode);
#endif
}

int hw_cmd_handler(int argc, char **argv)
{
    if (argc <= 1) {
        return 0;
    }
    if (!strcasecmp(argv[1], "cable")) {
        hw_exec_cable(argc, argv);
        return 0;
    }
    if (!strcasecmp(argv[1], "limit")) {
        hw_exec_limit(argc, argv);
        return 0;
    }
    if (!strcasecmp(argv[1], "ev")) {
        hw_exec_ev(argc, argv);
        return 0;
    }
    if (!strcasecmp(argv[1], "fault")) {
        hw_exec_fault(argc, argv);
        return 0;
    }
    return 0;
}

void sw_exec_nvs(int argc, char **argv)
{
    (void)argc;
    (void)argv;
#ifndef UNIT_TEST
    // Reserved: future console-driven NVS helpers.
#endif
}

void sw_exec_current(int argc, char **argv)
{
    if (argc <= 2) {
        return;
    }
    uint32_t data1 = 0;
    if (sscanf(argv[2], "%" SCNu32, &data1) <= 0) {
        return;
    }
    PRINTF_DEBUG("set charging current limit to %" PRIu32 " mA", data1);
#ifndef UNIT_TEST
    CT::Charger::ChargerManager::Controller().setChargingSessionCurrentLimit(data1);
#endif
}

int sw_cmd_handler(int argc, char **argv)
{
    if (argc <= 1) {
        return 0;
    }
    if (!strcasecmp(argv[1], "factoryreset")) {
#ifndef UNIT_TEST
        (void)esp_matter::factory_reset();
#endif
        return 0;
    }
    if (!strcasecmp(argv[1], "reboot")) {
#ifndef UNIT_TEST
        esp_restart();
#endif
        return 0;
    }
    if (!strcasecmp(argv[1], "device")) {
#ifndef UNIT_TEST
        CT::Charger::ChargerManager::Controller().showChargerDetails();
#endif
        return 0;
    }
    if (!strcasecmp(argv[1], "start")) {
#ifndef UNIT_TEST
        CT::Charger::ChargerManager::Controller().startChargingSession_WithCard("04482B6A116280");
#endif
        return 0;
    }
    if (!strcasecmp(argv[1], "stop")) {
#ifndef UNIT_TEST
        CT::Charger::ChargerManager::Controller().stopChargingSession_WithCard("04482B6A116280");
#endif
        return 0;
    }
    if (!strcasecmp(argv[1], "info")) {
#ifndef UNIT_TEST
        CT::Charger::ChargerManager::Controller().showChargingSessionInfo();
#endif
        return 0;
    }
    if (!strcasecmp(argv[1], "current")) {
        sw_exec_current(argc, argv);
        return 0;
    }
    return 0;
}

int nfc_cmd_handler(int argc, char **argv)
{
    if (argc > 1) {
        if (!strcmp(argv[1], "set")) {
#ifndef UNIT_TEST
            CT::Charger::ChargerManager::Controller().processDetectedCard("04482B6A116280");
#endif
        }
    }

    return 0;
}

void esp_charger_commands_register()
{
#ifndef UNIT_TEST
    const esp_console_cmd_t cmd_list[] = {
        {.command = "hw",
         .help = "emulator: hw cable <0|1> | limit <mA> | ev <0|1> | fault <0-255>",
         .func = hw_cmd_handler},
        {.command = "sw", .help = "control sw command", .func = sw_cmd_handler},
        {.command = "nfc", .help = "nfc commands", .func = nfc_cmd_handler},
    };

    // Loop to register each command
    for (size_t i = 0; i < std::size(cmd_list); i++) {
        esp_console_cmd_register(&cmd_list[i]);
    }
#endif
}
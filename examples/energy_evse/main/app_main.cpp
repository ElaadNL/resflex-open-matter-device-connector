// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <cinttypes>

#include "esp_err.h"
#include "esp_log.h"

#ifndef UNIT_TEST
#include "esp_console.h"

#include <nvs_flash.h>

#include <esp_matter.h>
#include <esp_matter_console.h>

#include <common_macros.h>
#include <app_reset.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "freertos/semphr.h"

#include <app/server/CommissioningWindowManager.h>
#include <app/server/Server.h>
#else
#include "esp_heap.h"
#include "nvs_flash.h"
#include "matterManager_unit_stub.h"
#endif

#include <helpers.h>

#include "app_cmd.h"

#include "hardwareControlInterface.h"
#include "chargerManager.h"
#ifndef UNIT_TEST
#include "matterManager.h"
#endif

extern "C" void app_main()
{
    uint32_t free_dram = esp_get_free_heap_size();
    uint32_t free_iram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL) - free_dram;
    DEBUG_CHECKPOINT("Free memory: %" PRIu32 " (DRAM/heap), %" PRIu32 " (IRAM)", free_dram, free_iram);
    DEBUG_CHECKPOINT("IDF version: %s", esp_get_idf_version());

#if 1
    /* Initialize the ESP NVS layer */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        PRINTF_DEBUG("NVS Error, will erased");
        err = nvs_flash_init();
    }

    /* Read err outside PRINTF_DEBUG so it is observed when verbose logging is compiled out */
    const bool nvs_flash_ok = (err == ESP_OK);
    PRINTF_DEBUG("Initializing NVS Flash %s", nvs_flash_ok ? "Done" : "Failed");
#else
    PRINTF_DEBUG("Initializing NVS Flash maybe in other modules ...");
#endif

    PRINTF_DEBUG("Initializing Application Driver ...");
    PRINTF_DEBUG("Initializing Main Charger ...");
    (void)CT::Charger::HardwareControlInterface::Instance().init();
    (void)CT::Charger::ChargerManager::Controller();
    (void)CT::Charger::MatterManager::GetInstance().Init();

    DEBUG_CHECKPOINT("Initializing Console Commands ...");
    esp_charger_commands_register();

#ifndef UNIT_TEST
#if CONFIG_ENABLE_CHIP_SHELL
    esp_matter::console::diagnostics_register_commands();
    esp_matter::console::wifi_register_commands();
    esp_matter::console::factoryreset_register_commands();
    esp_matter::console::init();
#endif
#endif
}

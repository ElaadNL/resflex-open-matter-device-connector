// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include "unity.h"

#include "test_charger_declarations.h"

void setUp(void)
{
}

void tearDown(void)
{
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    UNITY_BEGIN();

    run_test_charger_app_cmd_tests();
    run_test_charger_uiux_handler_tests();
    run_test_charger_manager_tests();
    run_test_charger_hardware_control_tests();
    run_test_charger_unit_other_tests();
    run_test_charger_session_math_tests();

    run_test_app_main_tests();
    run_test_nvs_helpers_tests();
    run_test_energy_time_utils_tests();
    run_test_time_sync_tests();
    run_test_power_topology_delegate_tests();
    run_test_power_topology_delegate_header_tests();
    run_test_energy_evse_mode_tests();
    run_test_esp32_device_instance_info_provider_tests();
    run_test_charging_targets_mem_mgr_tests();
    run_test_charger_manager_header_tests();
    run_test_matter_manager_tests();
    run_test_matter_manager_header_tests();
    run_test_dem_manufacturer_delegate_header_tests();
    run_test_device_energy_management_delegate_impl_tests();
    run_test_energy_evse_delegate_impl_tests();
    run_test_energy_evse_delegate_impl_header_tests();
    run_test_energy_evse_targets_store_tests();
    run_test_evse_targets_store_roundtrip_tests();
    run_test_tlv_roundtrip_tests();
    run_test_electrical_power_measurement_delegate_tests();
    run_test_electrical_power_measurement_delegate_header_tests();

    return UNITY_END();
}

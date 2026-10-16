// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include "unity.h"

#include "app_cmd.h"

static void test_esp_charger_commands_register_noop(void)
{
    esp_charger_commands_register();
}

static void test_hw_exec_cable_early_return_low_argc(void)
{
    char a0[] = "hw";
    char *argv[] = {a0};
    hw_exec_cable(1, argv);
}

static void test_hw_exec_cable_parses_zero(void)
{
    char a0[] = "hw";
    char a1[] = "cable";
    char a2[] = "0";
    char *argv[] = {a0, a1, a2};
    hw_exec_cable(3, argv);
}

static void test_hw_exec_cable_parses_nonzero(void)
{
    char a0[] = "hw";
    char a1[] = "cable";
    char a2[] = "1";
    char *argv[] = {a0, a1, a2};
    hw_exec_cable(3, argv);
}

static void test_hw_exec_limit_early_return(void)
{
    char a0[] = "hw";
    char *argv[] = {a0};
    hw_exec_limit(1, argv);
}

static void test_hw_exec_limit_parses_value(void)
{
    char a0[] = "hw";
    char a1[] = "limit";
    char a2[] = "32000";
    char *argv[] = {a0, a1, a2};
    hw_exec_limit(3, argv);
}

static void test_hw_exec_ev_early_return(void)
{
    char a0[] = "hw";
    char *argv[] = {a0};
    hw_exec_ev(1, argv);
}

static void test_hw_exec_ev_parses_drawing(void)
{
    char a0[] = "hw";
    char a1[] = "ev";
    char a2[] = "1";
    char *argv[] = {a0, a1, a2};
    hw_exec_ev(3, argv);
}

static void test_hw_exec_fault_early_return(void)
{
    char a0[] = "hw";
    char *argv[] = {a0};
    hw_exec_fault(1, argv);
}

static void test_hw_exec_fault_parses_code(void)
{
    char a0[] = "hw";
    char a1[] = "fault";
    char a2[] = "42";
    char *argv[] = {a0, a1, a2};
    hw_exec_fault(3, argv);
}

static void test_hw_cmd_handler_argc_le_one_returns_zero(void)
{
    char a0[] = "hw";
    char *argv[] = {a0};
    TEST_ASSERT_EQUAL_INT(0, hw_cmd_handler(1, argv));
}

static void test_hw_cmd_handler_cable_branch_case_insensitive(void)
{
    char a0[] = "hw";
    char a1[] = "CaBlE";
    char a2[] = "0";
    char *argv[] = {a0, a1, a2};
    TEST_ASSERT_EQUAL_INT(0, hw_cmd_handler(3, argv));
}

static void test_hw_cmd_handler_limit_branch(void)
{
    char a0[] = "hw";
    char a1[] = "LIMIT";
    char a2[] = "16000";
    char *argv[] = {a0, a1, a2};
    TEST_ASSERT_EQUAL_INT(0, hw_cmd_handler(3, argv));
}

static void test_hw_cmd_handler_ev_branch(void)
{
    char a0[] = "hw";
    char a1[] = "ev";
    char a2[] = "0";
    char *argv[] = {a0, a1, a2};
    TEST_ASSERT_EQUAL_INT(0, hw_cmd_handler(3, argv));
}

static void test_hw_cmd_handler_fault_branch(void)
{
    char a0[] = "hw";
    char a1[] = "fault";
    char a2[] = "255";
    char *argv[] = {a0, a1, a2};
    TEST_ASSERT_EQUAL_INT(0, hw_cmd_handler(3, argv));
}

static void test_hw_cmd_handler_unknown_subcommand_returns_zero(void)
{
    char a0[] = "hw";
    char a1[] = "unknown";
    char *argv[] = {a0, a1};
    TEST_ASSERT_EQUAL_INT(0, hw_cmd_handler(2, argv));
}

static void test_sw_exec_nvs_noop(void)
{
    char a0[] = "sw";
    char *argv[] = {a0};
    sw_exec_nvs(1, argv);
}

static void test_sw_exec_current_early_return_low_argc(void)
{
    char a0[] = "sw";
    char *argv[] = {a0};
    sw_exec_current(1, argv);
}

static void test_sw_exec_current_invalid_scanf_returns_early(void)
{
    char a0[] = "sw";
    char a1[] = "current";
    char a2[] = "not_a_number";
    char *argv[] = {a0, a1, a2};
    sw_exec_current(3, argv);
}

static void test_sw_exec_current_valid_uint32(void)
{
    char a0[] = "sw";
    char a1[] = "current";
    char a2[] = "16000";
    char *argv[] = {a0, a1, a2};
    sw_exec_current(3, argv);
}

static void test_sw_cmd_handler_argc_le_one(void)
{
    char a0[] = "sw";
    char *argv[] = {a0};
    TEST_ASSERT_EQUAL_INT(0, sw_cmd_handler(1, argv));
}

static void test_sw_cmd_handler_factoryreset(void)
{
    char a0[] = "sw";
    char a1[] = "FACTORYRESET";
    char *argv[] = {a0, a1};
    TEST_ASSERT_EQUAL_INT(0, sw_cmd_handler(2, argv));
}

static void test_sw_cmd_handler_reboot(void)
{
    char a0[] = "sw";
    char a1[] = "Reboot";
    char *argv[] = {a0, a1};
    TEST_ASSERT_EQUAL_INT(0, sw_cmd_handler(2, argv));
}

static void test_sw_cmd_handler_device(void)
{
    char a0[] = "sw";
    char a1[] = "device";
    char *argv[] = {a0, a1};
    TEST_ASSERT_EQUAL_INT(0, sw_cmd_handler(2, argv));
}

static void test_sw_cmd_handler_start(void)
{
    char a0[] = "sw";
    char a1[] = "start";
    char *argv[] = {a0, a1};
    TEST_ASSERT_EQUAL_INT(0, sw_cmd_handler(2, argv));
}

static void test_sw_cmd_handler_stop(void)
{
    char a0[] = "sw";
    char a1[] = "stop";
    char *argv[] = {a0, a1};
    TEST_ASSERT_EQUAL_INT(0, sw_cmd_handler(2, argv));
}

static void test_sw_cmd_handler_info(void)
{
    char a0[] = "sw";
    char a1[] = "INFO";
    char *argv[] = {a0, a1};
    TEST_ASSERT_EQUAL_INT(0, sw_cmd_handler(2, argv));
}

static void test_sw_cmd_handler_current_delegates(void)
{
    char a0[] = "sw";
    char a1[] = "current";
    char a2[] = "8000";
    char *argv[] = {a0, a1, a2};
    TEST_ASSERT_EQUAL_INT(0, sw_cmd_handler(3, argv));
}

static void test_sw_cmd_handler_unknown_returns_zero(void)
{
    char a0[] = "sw";
    char a1[] = "nosuch";
    char *argv[] = {a0, a1};
    TEST_ASSERT_EQUAL_INT(0, sw_cmd_handler(2, argv));
}

static void test_nfc_cmd_handler_argc_zero(void)
{
    TEST_ASSERT_EQUAL_INT(0, nfc_cmd_handler(0, nullptr));
}

static void test_nfc_cmd_handler_argc_one_no_crash(void)
{
    char a0[] = "nfc";
    char *argv[] = {a0};
    TEST_ASSERT_EQUAL_INT(0, nfc_cmd_handler(1, argv));
}

static void test_nfc_cmd_handler_set_branch(void)
{
    char a0[] = "nfc";
    char a1[] = "set";
    char *argv[] = {a0, a1};
    TEST_ASSERT_EQUAL_INT(0, nfc_cmd_handler(2, argv));
}

static void test_nfc_cmd_handler_non_set_branch(void)
{
    char a0[] = "nfc";
    char a1[] = "clear";
    char *argv[] = {a0, a1};
    TEST_ASSERT_EQUAL_INT(0, nfc_cmd_handler(2, argv));
}

static void test_nfc_cmd_handler_set_case_sensitive(void)
{
    char a0[] = "nfc";
    char a1[] = "SET";
    char *argv[] = {a0, a1};
    TEST_ASSERT_EQUAL_INT(0, nfc_cmd_handler(2, argv));
}

void run_test_charger_app_cmd_tests(void)
{
    RUN_TEST(test_esp_charger_commands_register_noop);
    RUN_TEST(test_hw_exec_cable_early_return_low_argc);
    RUN_TEST(test_hw_exec_cable_parses_zero);
    RUN_TEST(test_hw_exec_cable_parses_nonzero);
    RUN_TEST(test_hw_exec_limit_early_return);
    RUN_TEST(test_hw_exec_limit_parses_value);
    RUN_TEST(test_hw_exec_ev_early_return);
    RUN_TEST(test_hw_exec_ev_parses_drawing);
    RUN_TEST(test_hw_exec_fault_early_return);
    RUN_TEST(test_hw_exec_fault_parses_code);
    RUN_TEST(test_hw_cmd_handler_argc_le_one_returns_zero);
    RUN_TEST(test_hw_cmd_handler_cable_branch_case_insensitive);
    RUN_TEST(test_hw_cmd_handler_limit_branch);
    RUN_TEST(test_hw_cmd_handler_ev_branch);
    RUN_TEST(test_hw_cmd_handler_fault_branch);
    RUN_TEST(test_hw_cmd_handler_unknown_subcommand_returns_zero);
    RUN_TEST(test_sw_exec_nvs_noop);
    RUN_TEST(test_sw_exec_current_early_return_low_argc);
    RUN_TEST(test_sw_exec_current_invalid_scanf_returns_early);
    RUN_TEST(test_sw_exec_current_valid_uint32);
    RUN_TEST(test_sw_cmd_handler_argc_le_one);
    RUN_TEST(test_sw_cmd_handler_factoryreset);
    RUN_TEST(test_sw_cmd_handler_reboot);
    RUN_TEST(test_sw_cmd_handler_device);
    RUN_TEST(test_sw_cmd_handler_start);
    RUN_TEST(test_sw_cmd_handler_stop);
    RUN_TEST(test_sw_cmd_handler_info);
    RUN_TEST(test_sw_cmd_handler_current_delegates);
    RUN_TEST(test_sw_cmd_handler_unknown_returns_zero);
    RUN_TEST(test_nfc_cmd_handler_argc_zero);
    RUN_TEST(test_nfc_cmd_handler_argc_one_no_crash);
    RUN_TEST(test_nfc_cmd_handler_set_branch);
    RUN_TEST(test_nfc_cmd_handler_non_set_branch);
    RUN_TEST(test_nfc_cmd_handler_set_case_sensitive);
}

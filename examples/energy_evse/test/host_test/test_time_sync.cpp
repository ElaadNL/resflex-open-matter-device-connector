// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include "esp_sntp.h"
#include "nvs.h"
#include "TimeSync.h"

#include "unity.h"

static void test_init_configures_sntp(void)
{
    unit_test_nvs_reset();
    unit_test_sntp_reset();
    const char *servers[] = {"pool.ntp.org"};
    chip::Esp32TimeSync::Init(servers, 1, 1);
    TEST_ASSERT_TRUE(esp_sntp_enabled());
    TEST_ASSERT_EQUAL_STRING("pool.ntp.org", unit_test_time_sync_last_server());
    TEST_ASSERT_EQUAL_STRING("", unit_test_time_sync_server_at(1));
    TEST_ASSERT_EQUAL_STRING("", unit_test_time_sync_server_at(2));
}

static void test_init_configures_multiple_disclosed_servers(void)
{
    unit_test_nvs_reset();
    unit_test_sntp_reset();
    const char *servers[] = {"ntp.example.com", "ntp2.example.com", "ntp3.example.com"};
    chip::Esp32TimeSync::Init(servers, 3, 1);
    TEST_ASSERT_TRUE(esp_sntp_enabled());
    TEST_ASSERT_EQUAL_STRING("ntp.example.com", unit_test_time_sync_server_at(0));
    TEST_ASSERT_EQUAL_STRING("ntp2.example.com", unit_test_time_sync_server_at(1));
    TEST_ASSERT_EQUAL_STRING("ntp3.example.com", unit_test_time_sync_server_at(2));
}

static void test_init_skips_empty_servers_no_hardcoded_fallback(void)
{
    unit_test_nvs_reset();
    unit_test_sntp_reset();
    const char *servers[] = {"ntp.example.com", "", nullptr};
    chip::Esp32TimeSync::Init(servers, 3, 1);
    TEST_ASSERT_TRUE(esp_sntp_enabled());
    TEST_ASSERT_EQUAL_STRING("ntp.example.com", unit_test_time_sync_server_at(0));
    TEST_ASSERT_EQUAL_STRING("", unit_test_time_sync_server_at(1));
    TEST_ASSERT_EQUAL_STRING("", unit_test_time_sync_server_at(2));
}

static void test_init_rejects_zero_interval(void)
{
    unit_test_sntp_reset();
    const char *servers[] = {"pool.ntp.org"};
    chip::Esp32TimeSync::Init(servers, 1, 0);
    TEST_ASSERT_FALSE(esp_sntp_enabled());
}

static void test_init_skips_when_no_servers(void)
{
    unit_test_sntp_reset();
    chip::Esp32TimeSync::Init(nullptr, 0, 1);
    TEST_ASSERT_FALSE(esp_sntp_enabled());
}

static void test_time_sync_callback_persists_nvs(void)
{
    unit_test_nvs_reset();
    unit_test_sntp_reset();
    const char *servers[] = {"pool.ntp.org"};
    chip::Esp32TimeSync::Init(servers, 1, 1);
    unit_test_time_sync_fire_callback(1704067201LL, 0);
}

void run_test_time_sync_tests(void)
{
    RUN_TEST(test_init_configures_sntp);
    RUN_TEST(test_init_configures_multiple_disclosed_servers);
    RUN_TEST(test_init_skips_empty_servers_no_hardcoded_fallback);
    RUN_TEST(test_init_rejects_zero_interval);
    RUN_TEST(test_init_skips_when_no_servers);
    RUN_TEST(test_time_sync_callback_persists_nvs);
}

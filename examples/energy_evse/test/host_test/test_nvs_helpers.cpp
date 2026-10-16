// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include "nvs.h"

#include "unity.h"

namespace esp_matter {
namespace nvs_helpers {
esp_err_t set_device_type_in_nvs(uint8_t device_type_index);
esp_err_t get_device_type_from_nvs(uint8_t *device_type_index);
} // namespace nvs_helpers
} // namespace esp_matter

static void test_set_and_get_device_type(void)
{
    unit_test_nvs_reset();
    TEST_ASSERT_EQUAL(ESP_OK, esp_matter::nvs_helpers::set_device_type_in_nvs(42));
    uint8_t value = 0;
    TEST_ASSERT_EQUAL(ESP_OK, esp_matter::nvs_helpers::get_device_type_from_nvs(&value));
    TEST_ASSERT_EQUAL_UINT8(42, value);
}

void run_test_nvs_helpers_tests(void)
{
    RUN_TEST(test_set_and_get_device_type);
}

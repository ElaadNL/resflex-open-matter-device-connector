// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include "esp_heap.h"

uint32_t esp_get_free_heap_size(void)
{
    return 100000U;
}

uint32_t heap_caps_get_free_size(uint32_t caps)
{
    (void)caps;
    return 120000U;
}

const char *esp_get_idf_version(void)
{
    return "host-unit-test";
}

// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>

#define MALLOC_CAP_INTERNAL 0x1

uint32_t esp_get_free_heap_size(void);
uint32_t heap_caps_get_free_size(uint32_t caps);
const char *esp_get_idf_version(void);

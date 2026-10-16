// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "esp_err.h"

#include <cstdint>
#include <map>
#include <string>
#include <utility>

#ifndef CONFIG_CHIP_CONFIG_NAMESPACE_PARTITION_LABEL
#define CONFIG_CHIP_CONFIG_NAMESPACE_PARTITION_LABEL "nvs"
#endif

typedef uint32_t nvs_handle_t;

enum nvs_open_mode_t {
    NVS_READONLY = 0,
    NVS_READWRITE = 1,
};

void unit_test_nvs_reset(void);

esp_err_t nvs_open(const char *name, nvs_open_mode_t mode, nvs_handle_t *out_handle);
esp_err_t nvs_open_from_partition(const char *partition, const char *name, nvs_open_mode_t mode,
                                  nvs_handle_t *out_handle);
esp_err_t nvs_set_u8(nvs_handle_t handle, const char *key, uint8_t value);
esp_err_t nvs_get_u8(nvs_handle_t handle, const char *key, uint8_t *out_value);
esp_err_t nvs_get_i64(nvs_handle_t handle, const char *key, int64_t *out_value);
esp_err_t nvs_set_i64(nvs_handle_t handle, const char *key, int64_t value);
esp_err_t nvs_set_i32(nvs_handle_t handle, const char *key, int32_t value);
esp_err_t nvs_commit(nvs_handle_t handle);
void nvs_close(nvs_handle_t handle);

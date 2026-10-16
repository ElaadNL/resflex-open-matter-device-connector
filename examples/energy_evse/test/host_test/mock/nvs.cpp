// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include "nvs.h"

#include <atomic>

namespace {

struct Store {
    std::map<std::string, uint8_t> u8;
    std::map<std::string, int64_t> i64;
    std::map<std::string, int32_t> i32;
};

std::map<std::string, Store> &Db()
{
    static std::map<std::string, Store> db;
    return db;
}

std::atomic<nvs_handle_t> g_next_handle{1};

std::map<nvs_handle_t, std::string> &HandleNames()
{
    static std::map<nvs_handle_t, std::string> names;
    return names;
}

Store &StoreForHandle(nvs_handle_t handle)
{
    return Db()[HandleNames()[handle]];
}

} // namespace

void unit_test_nvs_reset(void)
{
    Db().clear();
    HandleNames().clear();
    g_next_handle = 1;
}

esp_err_t nvs_open(const char *name, nvs_open_mode_t mode, nvs_handle_t *out_handle)
{
    return nvs_open_from_partition(nullptr, name, mode, out_handle);
}

esp_err_t nvs_open_from_partition(const char *partition, const char *name, nvs_open_mode_t mode,
                                  nvs_handle_t *out_handle)
{
    (void)mode;
    if (out_handle == nullptr || name == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    const nvs_handle_t h = g_next_handle.fetch_add(1);
    *out_handle = h;
    const std::string key = partition ? std::string(partition) + ":" + name : std::string(name);
    HandleNames()[h] = key;
    if (Db().find(key) == Db().end()) {
        Db()[key] = Store{};
    }
    return ESP_OK;
}

esp_err_t nvs_set_u8(nvs_handle_t handle, const char *key, uint8_t value)
{
    if (key == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    StoreForHandle(handle).u8[key] = value;
    return ESP_OK;
}

esp_err_t nvs_get_u8(nvs_handle_t handle, const char *key, uint8_t *out_value)
{
    if (key == nullptr || out_value == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    auto &store = StoreForHandle(handle);
    auto it = store.u8.find(key);
    if (it == store.u8.end()) {
        return ESP_ERR_INVALID_ARG;
    }
    *out_value = it->second;
    return ESP_OK;
}

esp_err_t nvs_get_i64(nvs_handle_t handle, const char *key, int64_t *out_value)
{
    if (key == nullptr || out_value == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    auto &store = StoreForHandle(handle);
    auto it = store.i64.find(key);
    if (it == store.i64.end()) {
        return ESP_ERR_NOT_FOUND;
    }
    *out_value = it->second;
    return ESP_OK;
}

esp_err_t nvs_set_i64(nvs_handle_t handle, const char *key, int64_t value)
{
    if (key == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    StoreForHandle(handle).i64[key] = value;
    return ESP_OK;
}

esp_err_t nvs_set_i32(nvs_handle_t handle, const char *key, int32_t value)
{
    if (key == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    StoreForHandle(handle).i32[key] = value;
    return ESP_OK;
}

esp_err_t nvs_commit(nvs_handle_t handle)
{
    (void)handle;
    return ESP_OK;
}

void nvs_close(nvs_handle_t handle)
{
    HandleNames().erase(handle);
}

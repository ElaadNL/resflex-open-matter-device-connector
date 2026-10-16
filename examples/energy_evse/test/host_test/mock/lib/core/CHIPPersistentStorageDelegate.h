// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "chip_support.h"

#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace chip {

class PersistentStorageDelegate {
public:
    static constexpr size_t kKeyLengthMax = 32;

    virtual ~PersistentStorageDelegate() = default;

    virtual CHIP_ERROR SyncGetKeyValue(const char *key, void *buffer, uint16_t &size) = 0;
    virtual CHIP_ERROR SyncSetKeyValue(const char *key, const void *value, uint16_t size) = 0;
    virtual CHIP_ERROR SyncDeleteKeyValue(const char *key) = 0;
};

class InMemoryPersistentStorageDelegate : public PersistentStorageDelegate {
public:
    CHIP_ERROR SyncGetKeyValue(const char *key, void *buffer, uint16_t &size) override
    {
        if (key == nullptr) {
            return CHIP_ERROR_INVALID_ARGUMENT;
        }
        auto it = mStore.find(key);
        if (it == mStore.end()) {
            return CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND;
        }
        if (buffer == nullptr || size < it->second.size()) {
            if (buffer != nullptr && size > 0 && !it->second.empty()) {
                std::memcpy(buffer, it->second.data(), size);
            }
            size = static_cast<uint16_t>(it->second.size());
            return size < it->second.size() ? CHIP_ERROR_BUFFER_TOO_SMALL : CHIP_NO_ERROR;
        }
        std::memcpy(buffer, it->second.data(), it->second.size());
        size = static_cast<uint16_t>(it->second.size());
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR SyncSetKeyValue(const char *key, const void *value, uint16_t size) override
    {
        if (key == nullptr) {
            return CHIP_ERROR_INVALID_ARGUMENT;
        }
        if (size > 0 && value == nullptr) {
            return CHIP_ERROR_INVALID_ARGUMENT;
        }
        std::vector<uint8_t> bytes(size);
        if (size > 0) {
            std::memcpy(bytes.data(), value, size);
        }
        mStore[key] = std::move(bytes);
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR SyncDeleteKeyValue(const char *key) override
    {
        if (key == nullptr) {
            return CHIP_ERROR_INVALID_ARGUMENT;
        }
        mStore.erase(key);
        return CHIP_NO_ERROR;
    }

    void ClearForTest() { mStore.clear(); }

    bool TryGetCopyForTest(const char *key, std::vector<uint8_t> &out) const
    {
        auto it = mStore.find(key);
        if (it == mStore.end()) {
            return false;
        }
        out = it->second;
        return true;
    }

private:
    std::map<std::string, std::vector<uint8_t>> mStore;
};

} // namespace chip

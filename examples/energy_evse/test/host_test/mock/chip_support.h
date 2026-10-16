// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>

namespace chip {

constexpr uint32_t kChipEpochSecondsSinceUnixEpoch = 10957U * 24U * 60U * 60U;

using Percent = uint8_t;

struct CharSpan {
    const char *data = nullptr;
    size_t size = 0;

    static CharSpan fromCharString(const char *str)
    {
        CharSpan span;
        if (str != nullptr) {
            span.data = str;
            span.size = std::strlen(str);
        }
        return span;
    }
};

struct ByteSpan {
    const uint8_t *data = nullptr;
    size_t size = 0;
};

class ChipError {
public:
    constexpr ChipError(int code = 0)
        : mCode(code)
    {
    }

    constexpr operator int() const { return mCode; }

    constexpr bool operator==(const ChipError &other) const { return mCode == other.mCode; }
    constexpr bool operator!=(const ChipError &other) const { return mCode != other.mCode; }

    const char *Format() const { return "CHIP_ERROR"; }
    const char *AsString() const { return Format(); }

    int AsInteger() const { return mCode; }

private:
    int mCode;
};

enum ChipErrorCode : int {
    kChipNoError = 0,
    kChipErrorBufferTooSmall = 1,
    kChipErrorProviderListExhausted = 2,
    kChipErrorNotFound = 3,
    kChipErrorInvalidArgument = 4,
    kChipErrorIncorrectState = 5,
    kChipErrorUnsupportedChipFeature = 6,
    kChipErrorInternal = 7,
    kChipErrorNoMemory = 8,
    kChipErrorWrongKeyType = 9,
    kChipErrorBadRequest = 10,
    kChipErrorUninitialized = 11,
    kChipErrorUnexpectedTlvElement = 12,
    kChipErrorPersistedStorageFailed = 13,
    kChipErrorPersistedStorageValueNotFound = 14,
    kChipErrorRealTimeNotSynced = 15,
    kChipEndOfTlv = 16,
};

using CHIP_ERROR = ChipError;

constexpr CHIP_ERROR CHIP_NO_ERROR{kChipNoError};
constexpr CHIP_ERROR CHIP_ERROR_BUFFER_TOO_SMALL{kChipErrorBufferTooSmall};
constexpr CHIP_ERROR CHIP_ERROR_PROVIDER_LIST_EXHAUSTED{kChipErrorProviderListExhausted};
constexpr CHIP_ERROR CHIP_ERROR_NOT_FOUND{kChipErrorNotFound};
constexpr CHIP_ERROR CHIP_ERROR_INVALID_ARGUMENT{kChipErrorInvalidArgument};
constexpr CHIP_ERROR CHIP_ERROR_INCORRECT_STATE{kChipErrorIncorrectState};
constexpr CHIP_ERROR CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE{kChipErrorUnsupportedChipFeature};
constexpr CHIP_ERROR CHIP_ERROR_INTERNAL{kChipErrorInternal};
constexpr CHIP_ERROR CHIP_ERROR_NO_MEMORY{kChipErrorNoMemory};
constexpr CHIP_ERROR CHIP_ERROR_WRONG_KEY_TYPE{kChipErrorWrongKeyType};
constexpr CHIP_ERROR CHIP_ERROR_BAD_REQUEST{kChipErrorBadRequest};
constexpr CHIP_ERROR CHIP_ERROR_UNINITIALIZED{kChipErrorUninitialized};
constexpr CHIP_ERROR CHIP_ERROR_UNEXPECTED_TLV_ELEMENT{kChipErrorUnexpectedTlvElement};
constexpr CHIP_ERROR CHIP_ERROR_PERSISTED_STORAGE_FAILED{kChipErrorPersistedStorageFailed};
constexpr CHIP_ERROR CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND{kChipErrorPersistedStorageValueNotFound};
constexpr CHIP_ERROR CHIP_ERROR_REAL_TIME_NOT_SYNCED{kChipErrorRealTimeNotSynced};
constexpr CHIP_ERROR CHIP_END_OF_TLV{kChipEndOfTlv};

struct CHIP_ERROR_FORMAT {
    int code;
};

#define CHIP_ERROR_FORMAT "%d"

inline bool UnixEpochToChipEpochTime(uint32_t unixEpochTimeSeconds, uint32_t &outChipEpochTimeSeconds)
{
    if (unixEpochTimeSeconds < kChipEpochSecondsSinceUnixEpoch) {
        return false;
    }
    outChipEpochTimeSeconds = unixEpochTimeSeconds - kChipEpochSecondsSinceUnixEpoch;
    return true;
}

template <typename EnumType, typename Storage = uint32_t>
class BitMask {
public:
    BitMask()
        : mValue(0)
    {
    }
    explicit BitMask(EnumType e)
        : mValue(static_cast<Storage>(e))
    {
    }
    explicit BitMask(Storage v)
        : mValue(v)
    {
    }

    template <typename... Args>
    constexpr BitMask(EnumType flag, Args &&...args)
        : mValue(static_cast<Storage>(flag))
    {
        ((mValue |= static_cast<Storage>(args)), ...);
    }

    void Set(EnumType e) { mValue = static_cast<Storage>(e); }
    void Set(Storage v) { mValue = v; }

    BitMask &operator=(const BitMask &other)
    {
        mValue = other.mValue;
        return *this;
    }
    bool Has(EnumType e) const { return (mValue & static_cast<Storage>(e)) != 0; }
    bool HasAny() const { return mValue != 0; }

    bool HasAny(const BitMask &other) const { return (mValue & other.mValue) != 0; }

    Storage Raw() const { return mValue; }

    template <typename FieldEnum>
    uint8_t GetField(FieldEnum mask) const
    {
        return static_cast<uint8_t>(mValue & static_cast<Storage>(mask));
    }

private:
    Storage mValue = 0;
};

template <typename T>
class Optional {
public:
    Optional() = default;
    explicit Optional(T value)
        : mHasValue(true)
        , mValue(value)
    {
    }

    bool HasValue() const { return mHasValue; }

    T &Value() { return mValue; }
    const T &Value() const { return mValue; }

    void SetValue(const T &value)
    {
        mHasValue = true;
        mValue = value;
    }

    T ValueOr(T defaultValue) const { return mHasValue ? mValue : defaultValue; }

    void Clear() { mHasValue = false; }

    void ClearValue() { mHasValue = false; }

private:
    bool mHasValue = false;
    T mValue{};
};

template <typename T>
constexpr Optional<std::decay_t<T>> MakeOptional(T &&value)
{
    return Optional<std::decay_t<T>>(std::forward<T>(value));
}

template <typename T>
class Nullable {
public:
    Nullable() = default;

    explicit Nullable(T value) { SetNonNull(value); }

    bool IsNull() const { return mIsNull; }

    T &Value()
    {
        mIsNull = false;
        return mValue;
    }

    const T &Value() const { return mValue; }

    void SetNonNull(const T &value)
    {
        mValue = value;
        mIsNull = false;
    }

    void SetNull() { mIsNull = true; }

    T ValueOr(T defaultValue) const { return mIsNull ? defaultValue : mValue; }

    bool operator==(const Nullable &other) const
    {
        if (mIsNull != other.mIsNull) {
            return false;
        }
        return mIsNull || mValue == other.mValue;
    }

    bool operator!=(const Nullable &other) const { return !(*this == other); }

private:
    bool mIsNull = true;
    T mValue{};
};

struct MutableByteSpan {
    uint8_t *data = nullptr;
    size_t length = 0;
};

namespace Platform {
inline void CopyString(char *buf, size_t bufSize, const char *src)
{
    if (bufSize > 0) {
        std::strncpy(buf, src, bufSize - 1);
        buf[bufSize - 1] = '\0';
    }
}
} // namespace Platform

namespace System {
namespace Clock {
using Milliseconds64 = int64_t;
using Seconds32 = int32_t;
} // namespace Clock

class SystemClock {
public:
    void SetRealTimeMsForTest(int64_t ms) { real_time_ms_ = ms; }

    CHIP_ERROR GetClock_RealTimeMS(Clock::Milliseconds64 &ms) const
    {
        ms = real_time_ms_;
        return CHIP_NO_ERROR;
    }

private:
    int64_t real_time_ms_ = 0;
};

inline SystemClock &SystemClockInstance()
{
    static SystemClock clock;
    return clock;
}

inline SystemClock &SystemClock()
{
    return SystemClockInstance();
}

} // namespace System

#define VerifyOrDie(expr) \
    do {                  \
        if (!(expr)) {    \
        }                 \
    } while (0)

#define ReturnErrorCodeIf(cond, err) \
    do {                             \
        if (cond) {                  \
            return (err);            \
        }                            \
    } while (0)

#define ReturnErrorOnFailure(expr)      \
    do {                                \
        const CHIP_ERROR _err = (expr); \
        if (_err != CHIP_NO_ERROR) {    \
            return _err;                \
        }                               \
    } while (0)

#define VerifyOrReturnError(cond, err) \
    do {                               \
        if (!(cond)) {                 \
            return (err);              \
        }                              \
    } while (0)

#define VerifyOrReturnValue(cond, val) \
    do {                               \
        if (!(cond)) {                 \
            return (val);              \
        }                              \
    } while (0)

inline const char *ErrorStr(const ChipError &)
{
    return "CHIP_ERROR";
}

template <typename Enum>
constexpr std::underlying_type_t<Enum> to_underlying(Enum e)
{
    return static_cast<std::underlying_type_t<Enum>>(e);
}

} // namespace chip

using chip::CHIP_ERROR;
using chip::CHIP_NO_ERROR;
using chip::CHIP_ERROR_BUFFER_TOO_SMALL;
using chip::CHIP_ERROR_PROVIDER_LIST_EXHAUSTED;
using chip::CHIP_ERROR_NOT_FOUND;
using chip::CHIP_ERROR_INVALID_ARGUMENT;
using chip::CHIP_ERROR_INCORRECT_STATE;
using chip::CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
using chip::CHIP_ERROR_INTERNAL;
using chip::CHIP_ERROR_NO_MEMORY;
using chip::CHIP_ERROR_WRONG_KEY_TYPE;
using chip::CHIP_ERROR_BAD_REQUEST;
using chip::CHIP_ERROR_UNINITIALIZED;
using chip::CHIP_ERROR_UNEXPECTED_TLV_ELEMENT;
using chip::CHIP_ERROR_PERSISTED_STORAGE_FAILED;
using chip::CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND;
using chip::CHIP_ERROR_REAL_TIME_NOT_SYNCED;
using chip::CHIP_END_OF_TLV;
using chip::MutableByteSpan;
using chip::BitMask;
using chip::Nullable;
using chip::Optional;
using chip::Percent;
using chip::CharSpan;
using chip::ByteSpan;

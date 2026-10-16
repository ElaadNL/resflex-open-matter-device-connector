// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "app-common/zap-generated/cluster-enums.h"
#include "chip_support.h"

#include <cstdint>
#include <initializer_list>
#include <vector>

namespace chip {
namespace app {

namespace DataModel {

template <typename T>
using Nullable = chip::Nullable<T>;

template <typename T>
Nullable<T> MakeNullable(const T & value)
{
    Nullable<T> out;
    out.SetNonNull(value);
    return out;
}

template <typename T>
struct List {
    const T * data = nullptr;
    size_t count   = 0;

    List() = default;
    List(const T * d, size_t c) : data(d), count(c) {}

    template <typename U>
    List(const List<U> & other) : data(other.data), count(other.count)
    {}

    size_t size() const { return count; }

    const T * begin() const { return data; }
    const T * end() const { return data + count; }

    T * begin() { return const_cast<T *>(data); }
    T * end() { return const_cast<T *>(data) + count; }

    void reduce_size(size_t n) { count = n; }
};

template <typename T>
class DecodableList {
public:
    class Iterator {
    public:
        explicit Iterator(const DecodableList * parent) : mParent(parent) {}

        bool Next()
        {
            if (mParent == nullptr || mIndex >= mParent->mItems.size()) {
                return false;
            }
            ++mIndex;
            return mIndex <= mParent->mItems.size();
        }

        T & GetValue()
        {
            auto * parent = const_cast<DecodableList *>(mParent);
            return parent->mItems[mIndex > 0 ? mIndex - 1 : 0];
        }

    private:
        const DecodableList * mParent;
        size_t mIndex = 0;
    };

    DecodableList() = default;

    explicit DecodableList(std::initializer_list<T> items) : mItems(items) {}

    CHIP_ERROR ComputeSize(size_t * out) const
    {
        if (out == nullptr) {
            return CHIP_ERROR_INVALID_ARGUMENT;
        }
        if (mComputeSizeError != CHIP_NO_ERROR) {
            return mComputeSizeError;
        }
        *out = mItems.size();
        return CHIP_NO_ERROR;
    }

    Iterator begin() const { return Iterator(this); }

    void SetItemsForTest(std::initializer_list<T> items) { mItems.assign(items); }

    void SetComputeSizeErrorForTest(CHIP_ERROR err) { mComputeSizeError = err; }

    CHIP_ERROR GetComputeSizeErrorForTest() const { return mComputeSizeError; }

private:
    std::vector<T> mItems;
    CHIP_ERROR mComputeSizeError = CHIP_NO_ERROR;
};

} // namespace DataModel

namespace Clusters {
namespace EnergyEvse {
namespace Structs {

namespace ChargingTargetStruct {

struct Type {
    uint16_t targetTimeMinutesPastMidnight = 0;
    Optional<Percent> targetSoC;
    Optional<int64_t> addedEnergy;
};

using DecodableType = Type;

} // namespace ChargingTargetStruct

namespace ChargingTargetScheduleStruct {

struct Type {
    BitMask<TargetDayOfWeekBitmap> dayOfWeekForSequence;
    DataModel::List<const ChargingTargetStruct::Type> chargingTargets;
};

struct DecodableType {
    BitMask<TargetDayOfWeekBitmap> dayOfWeekForSequence;
    DataModel::DecodableList<ChargingTargetStruct::DecodableType> chargingTargets;
};

} // namespace ChargingTargetScheduleStruct

} // namespace Structs
} // namespace EnergyEvse

namespace DeviceEnergyManagement {

enum class AdjustmentCauseEnum : uint8_t { kLocalOptimization = 0, kGridOptimization = 1, kUnknownEnumValue = 0xFF };
enum class CauseEnum : uint8_t { kNormalCompletion = 0, kCancelled = 1, kUserOptOut = 2 };

namespace Structs {

struct SlotAdjustmentStruct {
    struct DecodableType {
        uint16_t slotIndex = 0;
    };
};

struct ConstraintsStruct {
    struct DecodableType {
        int64_t power = 0;
    };
};

} // namespace Structs
} // namespace DeviceEnergyManagement

} // namespace Clusters
} // namespace app
} // namespace chip

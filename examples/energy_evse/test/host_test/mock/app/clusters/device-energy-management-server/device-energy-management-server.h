// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "app-common/zap-generated/cluster-objects.h"
#include "chip_support.h"
#include "protocols/interaction_model/StatusCode.h"

#include <cstdint>

namespace chip {
using EndpointId = uint16_t;
using ClusterId = uint32_t;
using AttributeId = uint32_t;

namespace app {
namespace Clusters {
namespace DeviceEnergyManagement {

constexpr ClusterId Id = 0x0094;

enum class Feature : uint32_t {
    kPowerAdjustment = 0x01,
};

enum class ESATypeEnum : uint8_t { kEvse = 0, kSpaceHeater = 1, kUnknownEnumValue = 0xFF };
enum class ESAStateEnum : uint8_t {
    kOffline = 0,
    kOnline = 1,
    kPowerAdjustActive = 2,
    kPaused = 3,
    kUnknownEnumValue = 0xFF
};
enum class OptOutStateEnum : uint8_t { kNoOptOut = 0, kLocalOptOut = 1, kGridOptOut = 2, kOptOut = 3 };
enum class PowerAdjustReasonEnum : uint8_t {
    kNoAdjustment = 0,
    kLocalOptimizationAdjustment = 1,
    kGridOptimizationAdjustment = 2,
};
enum class ForecastUpdateReasonEnum : uint8_t {
    kInternalOptimization = 0,
    kLocalOptimization = 1,
    kGridOptimization = 2,
    kUnknownEnumValue = 0xFF,
};

namespace Structs {
struct PowerAdjustCapabilityStruct {
    struct Type {
        PowerAdjustReasonEnum cause = PowerAdjustReasonEnum::kNoAdjustment;
    };
};
struct ForecastStruct {
    struct Type {
        uint32_t forecastID = 0;
        uint32_t startTime = 0;
        uint32_t endTime = 0;
        ForecastUpdateReasonEnum forecastUpdateReason = ForecastUpdateReasonEnum::kInternalOptimization;
    };
};
} // namespace Structs

namespace Events {
struct PowerAdjustStart {
    struct Type {
    };
};
struct PowerAdjustEnd {
    struct Type {
        CauseEnum cause = CauseEnum::kNormalCompletion;
        uint32_t duration = 0;
        int64_t energyUse = 0;
    };
};
struct Paused {
    struct Type {
    };
};
struct Resumed {
    struct Type {
        CauseEnum cause = CauseEnum::kNormalCompletion;
    };
};
} // namespace Events

namespace Attributes {
struct ESAType {
    static constexpr AttributeId Id = 0;
};
struct ESACanGenerate {
    static constexpr AttributeId Id = 1;
};
struct ESAState {
    static constexpr AttributeId Id = 2;
};
struct AbsMinPower {
    static constexpr AttributeId Id = 3;
};
struct AbsMaxPower {
    static constexpr AttributeId Id = 4;
};
struct PowerAdjustmentCapability {
    static constexpr AttributeId Id = 5;
};
struct Forecast {
    static constexpr AttributeId Id = 6;
};
struct OptOutState {
    static constexpr AttributeId Id = 7;
};
} // namespace Attributes

class Delegate {
public:
    virtual ~Delegate() = default;

    virtual chip::Protocols::InteractionModel::Status PowerAdjustRequest(int64_t, uint32_t, AdjustmentCauseEnum) = 0;
    virtual chip::Protocols::InteractionModel::Status CancelPowerAdjustRequest() = 0;
    virtual chip::Protocols::InteractionModel::Status StartTimeAdjustRequest(uint32_t, AdjustmentCauseEnum) = 0;
    virtual chip::Protocols::InteractionModel::Status PauseRequest(uint32_t, AdjustmentCauseEnum) = 0;
    virtual chip::Protocols::InteractionModel::Status ResumeRequest() = 0;
    virtual chip::Protocols::InteractionModel::Status ModifyForecastRequest(
        uint32_t, const DataModel::DecodableList<Structs::SlotAdjustmentStruct::DecodableType> &,
        AdjustmentCauseEnum) = 0;
    virtual chip::Protocols::InteractionModel::Status RequestConstraintBasedForecast(
        const DataModel::DecodableList<Structs::ConstraintsStruct::DecodableType> &, AdjustmentCauseEnum) = 0;
    virtual chip::Protocols::InteractionModel::Status CancelRequest() = 0;

    virtual ESATypeEnum GetESAType() = 0;
    virtual bool GetESACanGenerate() = 0;
    virtual ESAStateEnum GetESAState() = 0;
    virtual int64_t GetAbsMinPower() = 0;
    virtual int64_t GetAbsMaxPower() = 0;
    virtual const DataModel::Nullable<Structs::PowerAdjustCapabilityStruct::Type> &GetPowerAdjustmentCapability() = 0;
    virtual const DataModel::Nullable<Structs::ForecastStruct::Type> &GetForecast() = 0;
    virtual OptOutStateEnum GetOptOutState() = 0;

    virtual CHIP_ERROR SetESAState(ESAStateEnum) = 0;

    EndpointId mEndpointId = 0;

protected:
    void SetEndpointId(EndpointId id) { mEndpointId = id; }
};

class Instance {
public:
    bool HasFeature(Feature feature) const { return feature_ == feature; }

    void SetFeatureForTest(Feature feature) { feature_ = feature; }

private:
    Feature feature_ = Feature::kPowerAdjustment;
};

} // namespace DeviceEnergyManagement
} // namespace Clusters
} // namespace app
} // namespace chip

template <typename E>
inline uint8_t to_underlying(E e)
{
    return static_cast<uint8_t>(e);
}

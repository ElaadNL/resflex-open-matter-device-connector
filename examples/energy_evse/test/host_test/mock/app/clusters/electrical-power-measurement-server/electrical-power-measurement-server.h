// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "app-common/zap-generated/cluster-objects.h"
#include "app/util/af-types.h"
#include "chip_support.h"

#include <cstdint>
#include <utility>

namespace chip {
using Percent100ths = uint16_t;

namespace app {
namespace Clusters {
namespace detail {
enum class MeasurementTypeEnum : uint16_t {
    kUnspecified = 0x00,
    kVoltage = 0x01,
    kActiveCurrent = 0x02,
    kReactiveCurrent = 0x03,
    kApparentCurrent = 0x04,
    kActivePower = 0x05,
    kReactivePower = 0x06,
    kApparentPower = 0x07,
    kRMSVoltage = 0x08,
    kRMSCurrent = 0x09,
    kRMSPower = 0x0A,
    kFrequency = 0x0B,
    kPowerFactor = 0x0C,
    kNeutralCurrent = 0x0D,
    kUnknownEnumValue = 15,
};
} // namespace detail

namespace ElectricalPowerMeasurement {

static constexpr ClusterId Id = 0x00000090;

using MeasurementTypeEnum = detail::MeasurementTypeEnum;

enum class PowerModeEnum : uint8_t {
    kUnknown = 0x00,
    kDc = 0x01,
    kAc = 0x02,
    kUnknownEnumValue = 3,
};

enum class Feature : uint32_t {
    kDirectCurrent = 0x1,
    kAlternatingCurrent = 0x2,
    kPolyphasePower = 0x4,
    kHarmonics = 0x8,
    kPowerQuality = 0x10,
};

enum class OptionalAttributes : uint32_t {
    kOptionalAttributeRanges = 0x1,
    kOptionalAttributeVoltage = 0x2,
    kOptionalAttributeActiveCurrent = 0x4,
};

inline PowerModeEnum EnsureKnownEnumValue(PowerModeEnum val)
{
    switch (val) {
    case PowerModeEnum::kUnknown:
    case PowerModeEnum::kDc:
    case PowerModeEnum::kAc:
        return val;
    default:
        return PowerModeEnum::kUnknownEnumValue;
    }
}

namespace Structs {

namespace MeasurementAccuracyRangeStruct {
struct Type {
    int64_t rangeMin = 0;
    int64_t rangeMax = 0;
    Optional<Percent100ths> percentMax;
    Optional<Percent100ths> percentMin;
    Optional<Percent100ths> percentTypical;
};
using DecodableType = Type;
} // namespace MeasurementAccuracyRangeStruct

namespace MeasurementAccuracyStruct {
struct Type {
    MeasurementTypeEnum measurementType = MeasurementTypeEnum::kUnspecified;
    bool measured = false;
    int64_t minMeasuredValue = 0;
    int64_t maxMeasuredValue = 0;
    DataModel::List<const MeasurementAccuracyRangeStruct::Type> accuracyRanges;
};
using DecodableType = Type;
} // namespace MeasurementAccuracyStruct

namespace MeasurementRangeStruct {
struct Type {
    MeasurementTypeEnum measurementType = MeasurementTypeEnum::kUnspecified;
    int64_t min = 0;
    int64_t max = 0;
};
using DecodableType = Type;
} // namespace MeasurementRangeStruct

namespace HarmonicMeasurementStruct {
struct Type {
    uint8_t order = 0;
    DataModel::Nullable<int64_t> measurement;
};
using DecodableType = Type;
} // namespace HarmonicMeasurementStruct

} // namespace Structs

namespace Attributes {

namespace PowerMode {
static constexpr AttributeId Id = 0x00000000;
}
namespace NumberOfMeasurementTypes {
static constexpr AttributeId Id = 0x00000001;
}
namespace Accuracy {
static constexpr AttributeId Id = 0x00000002;
}
namespace Ranges {
static constexpr AttributeId Id = 0x00000003;
}
namespace Voltage {
static constexpr AttributeId Id = 0x00000004;
}
namespace ActiveCurrent {
static constexpr AttributeId Id = 0x00000005;
}
namespace ReactiveCurrent {
static constexpr AttributeId Id = 0x00000006;
}
namespace ApparentCurrent {
static constexpr AttributeId Id = 0x00000007;
}
namespace ActivePower {
static constexpr AttributeId Id = 0x00000008;
}
namespace ReactivePower {
static constexpr AttributeId Id = 0x00000009;
}
namespace ApparentPower {
static constexpr AttributeId Id = 0x0000000A;
}
namespace RMSVoltage {
static constexpr AttributeId Id = 0x0000000B;
}
namespace RMSCurrent {
static constexpr AttributeId Id = 0x0000000C;
}
namespace RMSPower {
static constexpr AttributeId Id = 0x0000000D;
}
namespace Frequency {
static constexpr AttributeId Id = 0x0000000E;
}
namespace HarmonicCurrents {
static constexpr AttributeId Id = 0x0000000F;
}
namespace HarmonicPhases {
static constexpr AttributeId Id = 0x00000010;
}
namespace PowerFactor {
static constexpr AttributeId Id = 0x00000011;
}
namespace NeutralCurrent {
static constexpr AttributeId Id = 0x00000012;
}

} // namespace Attributes

class Delegate {
public:
    virtual ~Delegate() = default;

    void SetEndpointId(EndpointId aEndpoint) { mEndpointId = aEndpoint; }
    EndpointId GetEndpointId() const { return mEndpointId; }

    virtual PowerModeEnum GetPowerMode() = 0;
    virtual uint8_t GetNumberOfMeasurementTypes() = 0;

    virtual CHIP_ERROR StartAccuracyRead() = 0;
    virtual CHIP_ERROR GetAccuracyByIndex(uint8_t, Structs::MeasurementAccuracyStruct::Type &) = 0;
    virtual CHIP_ERROR EndAccuracyRead() = 0;

    virtual CHIP_ERROR StartRangesRead() = 0;
    virtual CHIP_ERROR GetRangeByIndex(uint8_t, Structs::MeasurementRangeStruct::Type &) = 0;
    virtual CHIP_ERROR EndRangesRead() = 0;

    virtual CHIP_ERROR StartHarmonicCurrentsRead() = 0;
    virtual CHIP_ERROR GetHarmonicCurrentsByIndex(uint8_t, Structs::HarmonicMeasurementStruct::Type &) = 0;
    virtual CHIP_ERROR EndHarmonicCurrentsRead() = 0;

    virtual CHIP_ERROR StartHarmonicPhasesRead() = 0;
    virtual CHIP_ERROR GetHarmonicPhasesByIndex(uint8_t, Structs::HarmonicMeasurementStruct::Type &) = 0;
    virtual CHIP_ERROR EndHarmonicPhasesRead() = 0;

    virtual DataModel::Nullable<int64_t> GetVoltage() = 0;
    virtual DataModel::Nullable<int64_t> GetActiveCurrent() = 0;
    virtual DataModel::Nullable<int64_t> GetReactiveCurrent() = 0;
    virtual DataModel::Nullable<int64_t> GetApparentCurrent() = 0;
    virtual DataModel::Nullable<int64_t> GetActivePower() = 0;
    virtual DataModel::Nullable<int64_t> GetReactivePower() = 0;
    virtual DataModel::Nullable<int64_t> GetApparentPower() = 0;
    virtual DataModel::Nullable<int64_t> GetRMSVoltage() = 0;
    virtual DataModel::Nullable<int64_t> GetRMSCurrent() = 0;
    virtual DataModel::Nullable<int64_t> GetRMSPower() = 0;
    virtual DataModel::Nullable<int64_t> GetFrequency() = 0;
    virtual DataModel::Nullable<int64_t> GetPowerFactor() = 0;
    virtual DataModel::Nullable<int64_t> GetNeutralCurrent() = 0;

protected:
    EndpointId mEndpointId = 0;
};

class Instance {
public:
    Instance(EndpointId aEndpointId, Delegate &aDelegate, BitMask<Feature> aFeature,
             BitMask<OptionalAttributes> aOptionalAttributes)
    {
        (void)aFeature;
        (void)aOptionalAttributes;
        aDelegate.SetEndpointId(aEndpointId);
    }

    virtual ~Instance() = default;

    CHIP_ERROR Init() { return CHIP_NO_ERROR; }
    void Shutdown() {}
};

} // namespace ElectricalPowerMeasurement
} // namespace Clusters
} // namespace app
} // namespace chip

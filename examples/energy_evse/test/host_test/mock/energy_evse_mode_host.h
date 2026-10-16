// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "app-common/zap-generated/cluster-objects.h"
#include "chip_support.h"

#include <cstring>
#include <iterator>
#include <utility>

namespace chip {

struct MutableCharSpan {
    char *data = nullptr;
    size_t size = 0;
};

inline CHIP_ERROR CopyCharSpanToMutableCharSpan(CharSpan src, MutableCharSpan &dst)
{
    if (src.size > dst.size) {
        return CHIP_ERROR_BUFFER_TOO_SMALL;
    }
    if (src.size > 0 && src.data != nullptr && dst.data != nullptr) {
        std::memcpy(dst.data, src.data, src.size);
    }
    return CHIP_NO_ERROR;
}

namespace app {
namespace Clusters {
namespace detail {
namespace Structs {
struct ModeTagStruct {
    struct Type {
        uint8_t value = 0;
    };
};
struct ModeOptionStruct {
    struct Type {
        CharSpan label;
        uint8_t mode = 0;
        DataModel::List<const ModeTagStruct::Type> modeTags;
    };
};
} // namespace Structs
} // namespace detail

namespace EnergyEvseMode {

enum class ModeTag : uint8_t { kManual = 0, kTimeOfUse = 1, kSolarCharging = 2 };

constexpr uint8_t kModeManual = 0;
constexpr uint8_t kModeTimeOfUse = 1;
constexpr uint8_t kModeSolarCharging = 2;
constexpr uint8_t kModeTimeOfUseAndSolarCharging = 3;

namespace ModeBase {
enum class StatusCode : uint8_t { kSuccess = 0 };

namespace Commands {
struct ChangeToModeResponse {
    struct Type {
        uint8_t status = 0;
    };
};
} // namespace Commands

class Delegate {
public:
    virtual ~Delegate() = default;
    virtual CHIP_ERROR Init() = 0;
    virtual void HandleChangeToMode(uint8_t, Commands::ChangeToModeResponse::Type &) = 0;
    virtual CHIP_ERROR GetModeLabelByIndex(uint8_t, MutableCharSpan &) = 0;
    virtual CHIP_ERROR GetModeValueByIndex(uint8_t, uint8_t &) = 0;
    virtual CHIP_ERROR GetModeTagsByIndex(uint8_t, DataModel::List<detail::Structs::ModeTagStruct::Type> &) = 0;
};
} // namespace ModeBase

class EnergyEvseModeDelegate : public ModeBase::Delegate {
public:
    ~EnergyEvseModeDelegate() override = default;

    CHIP_ERROR Init() override;
    void HandleChangeToMode(uint8_t NewMode, ModeBase::Commands::ChangeToModeResponse::Type &response) override;
    CHIP_ERROR GetModeLabelByIndex(uint8_t modeIndex, MutableCharSpan &label) override;
    CHIP_ERROR GetModeValueByIndex(uint8_t modeIndex, uint8_t &value) override;
    CHIP_ERROR GetModeTagsByIndex(uint8_t modeIndex,
                                  DataModel::List<detail::Structs::ModeTagStruct::Type> &tags) override;

private:
    using ModeTagStructType = detail::Structs::ModeTagStruct::Type;
    ModeTagStructType ModeTagsManual[1] = {{0}};
    ModeTagStructType ModeTagsTimeOfUse[1] = {{1}};
    ModeTagStructType ModeTagsSolarCharging[1] = {{2}};
    ModeTagStructType ModeTagsTimeOfUseAndSolarCharging[2] = {{1}, {2}};

    const detail::Structs::ModeOptionStruct::Type kModeOptions[4] = {
        {CharSpan::fromCharString("Manual"), kModeManual, DataModel::List<const ModeTagStructType>(ModeTagsManual, 1)},
        {CharSpan::fromCharString("Auto-scheduled"), kModeTimeOfUse,
         DataModel::List<const ModeTagStructType>(ModeTagsTimeOfUse, 1)},
        {CharSpan::fromCharString("Solar"), kModeSolarCharging,
         DataModel::List<const ModeTagStructType>(ModeTagsSolarCharging, 1)},
        {CharSpan::fromCharString("Auto-scheduled with Solar charging"), kModeTimeOfUseAndSolarCharging,
         DataModel::List<const ModeTagStructType>(ModeTagsTimeOfUseAndSolarCharging, 2)},
    };
};

} // namespace EnergyEvseMode
} // namespace Clusters
} // namespace app
} // namespace chip

inline uint8_t to_underlying(chip::app::Clusters::EnergyEvseMode::ModeBase::StatusCode e)
{
    return static_cast<uint8_t>(e);
}

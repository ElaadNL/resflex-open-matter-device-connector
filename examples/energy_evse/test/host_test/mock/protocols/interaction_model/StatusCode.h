// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>

namespace chip {
namespace Protocols {
namespace InteractionModel {

enum class Status : uint8_t {
    Success = 0x00,
    Failure = 0x01,
    InvalidSubscription = 0x7d,
    UnsupportedAccess = 0x7e,
    UnsupportedEndpoint = 0x7f,
    InvalidAction = 0x80,
    UnsupportedCommand = 0x81,
    UnsupportedAttribute = 0x86,
    ConstraintError = 0x87,
};

} // namespace InteractionModel
} // namespace Protocols
} // namespace chip

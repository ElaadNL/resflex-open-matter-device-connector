// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <cstdint>

namespace chip {
namespace Esp32TimeSync {

inline constexpr size_t kMaxConfiguredNtpServers = 3;

void Init(const char *const *serverNames, size_t serverCount, uint16_t syncIntervalDay);

} // namespace Esp32TimeSync
} // namespace chip

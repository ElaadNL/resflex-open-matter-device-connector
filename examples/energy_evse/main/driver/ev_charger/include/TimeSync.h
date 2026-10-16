/*
 *
 *    Copyright (c) 2024 Project CHIP Authors
 *    All rights reserved.
 *    SPDX-FileCopyrightText: ElaadNL
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace chip {
namespace Esp32TimeSync {

/** Maximum NTP destinations accepted by the SNTP client (must match lwIP SNTP_MAX_SERVERS). */
inline constexpr size_t kMaxConfiguredNtpServers = 3;

/**
 * Initialize SNTP using only the caller-supplied NTP destinations.
 *
 * Empty or null entries are skipped. There are no hardcoded fallback servers.
 * Plain SNTP is unauthenticated; prefer Matter Time Synchronization (CASE) when available.
 *
 * @param serverNames    Array of NTP hostnames (lifetime need not outlive this call).
 * @param serverCount    Number of entries in serverNames (excess beyond kMaxConfiguredNtpServers ignored).
 * @param syncIntervalDay Resync interval in days; must be non-zero.
 */
void Init(const char *const *serverNames, size_t serverCount, uint16_t syncIntervalDay);

} // namespace Esp32TimeSync
} // namespace chip

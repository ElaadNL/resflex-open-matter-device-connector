// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#define ChipLogFormatX64 "%lld"
#define ChipLogValueX64(x) (static_cast<long long>(x))

#define ChipLogError(module, ...) ((void)0)
#define ChipLogProgress(module, ...) ((void)0)
#define ChipLogDetail(module, ...) ((void)0)

namespace chip {
namespace Logging {
enum LogModule {
    AppServer,
};
} // namespace Logging
} // namespace chip

using chip::Logging::AppServer;

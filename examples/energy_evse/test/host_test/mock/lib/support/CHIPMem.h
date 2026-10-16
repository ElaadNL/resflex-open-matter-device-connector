// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "chip_platform.h"
#include "lib/support/ScopedBuffer.h"

namespace chip {
namespace Platform {

template <typename T>
using ScopedMemoryBuffer = chip::ScopedMemoryBuffer<T>;

} // namespace Platform
} // namespace chip

// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "chip_support.h"

namespace chip {
namespace app {
namespace Clusters {
namespace ModeBase {

class Delegate {
public:
    virtual ~Delegate() = default;
};

} // namespace ModeBase
} // namespace Clusters
} // namespace app
} // namespace chip

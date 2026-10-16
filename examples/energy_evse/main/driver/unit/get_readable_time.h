// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <cstddef>

void GetReadableTime(uint32_t matterEpoch_s, char *outStr, size_t outSize);

// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>

/// Host tests may pin PRNG output for deterministic meter current.
inline uint32_t &unit_test_esp_random_next()
{
    static uint32_t v = 500U;
    return v;
}

inline uint32_t esp_random(void)
{
    return unit_test_esp_random_next();
}

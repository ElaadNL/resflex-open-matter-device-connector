// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "chip_support.h"

#include <cstdlib>
#include <cstring>

namespace chip {
namespace Platform {

inline bool &ForceAllocFail()
{
    static bool fail = false;
    return fail;
}

inline void *MemoryAlloc(size_t size)
{
    if (ForceAllocFail()) {
        return nullptr;
    }
    return std::malloc(size);
}

inline void MemoryFree(void *ptr)
{
    std::free(ptr);
}

inline void Delete(void *ptr)
{
    MemoryFree(ptr);
}

} // namespace Platform
} // namespace chip

inline void unit_test_platform_force_alloc_fail(bool fail)
{
    chip::Platform::ForceAllocFail() = fail;
}

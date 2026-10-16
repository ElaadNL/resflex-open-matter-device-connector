// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "chip_platform.h"

#include <cstddef>
#include <utility>

namespace chip {

template <typename T>
class ScopedMemoryBuffer {
public:
    ScopedMemoryBuffer() = default;
    ScopedMemoryBuffer(ScopedMemoryBuffer &&other) noexcept { *this = std::move(other); }

    ScopedMemoryBuffer &operator=(ScopedMemoryBuffer &&other) noexcept
    {
        if (this != &other) {
            Free();
            mBuffer = other.mBuffer;
            other.mBuffer = nullptr;
        }
        return *this;
    }

    ~ScopedMemoryBuffer() { Free(); }

    explicit operator bool() const { return mBuffer != nullptr; }

    T *Get() { return mBuffer; }
    const T *Get() const { return mBuffer; }

    T *Release()
    {
        T *ptr = mBuffer;
        mBuffer = nullptr;
        return ptr;
    }

    ScopedMemoryBuffer &Calloc(size_t elementCount)
    {
        Free();
        mBuffer = static_cast<T *>(Platform::MemoryAlloc(elementCount * sizeof(T)));
        return *this;
    }

    void Free()
    {
        if (mBuffer != nullptr) {
            Platform::MemoryFree(mBuffer);
            mBuffer = nullptr;
        }
    }

private:
    T *mBuffer = nullptr;
};

} // namespace chip

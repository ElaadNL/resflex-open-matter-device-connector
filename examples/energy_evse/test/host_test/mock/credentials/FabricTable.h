// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "chip_support.h"

#include <cstdint>
#include <vector>

namespace chip {

using FabricIndex = uint8_t;

class FabricTable {
public:
    class Delegate {
    public:
        virtual ~Delegate() = default;
        virtual void OnFabricRemoved(const FabricTable &fabricTable, FabricIndex fabricIndex) = 0;
    };

    unsigned FabricCount() const { return mFabricCount; }

    void SetFabricCountForTest(unsigned count) { mFabricCount = count; }

    CHIP_ERROR AddFabricDelegate(Delegate *delegate)
    {
        if (delegate != nullptr) {
            mDelegates.push_back(delegate);
        }
        return CHIP_NO_ERROR;
    }

    void RemoveFabricDelegate(Delegate *delegate)
    {
        for (auto it = mDelegates.begin(); it != mDelegates.end(); ++it) {
            if (*it == delegate) {
                mDelegates.erase(it);
                break;
            }
        }
    }

private:
    unsigned mFabricCount = 0;
    std::vector<Delegate *> mDelegates;
};

} // namespace chip

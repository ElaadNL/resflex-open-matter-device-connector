// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "chip_support.h"
#include "credentials/FabricTable.h"
#include "lib/core/CHIPPersistentStorageDelegate.h"

namespace chip {

enum class CommissioningWindowAdvertisement : uint8_t {
    kAllSupported = 0,
};

class CommissioningWindowManager {
public:
    bool IsCommissioningWindowOpen() const { return mOpen; }

    CHIP_ERROR OpenBasicCommissioningWindow(System::Clock::Seconds32, CommissioningWindowAdvertisement)
    {
        if (mOpenFail) {
            return CHIP_ERROR_INTERNAL;
        }
        mOpen = true;
        return CHIP_NO_ERROR;
    }

    void SetCommissioningOpenFailForTest(bool fail) { mOpenFail = fail; }

    void ResetForTest()
    {
        mOpen = false;
        mOpenFail = false;
    }

private:
    bool mOpen = false;
    bool mOpenFail = false;
};

class Server {
public:
    static Server &GetInstance()
    {
        static Server inst;
        return inst;
    }

    FabricTable &GetFabricTable() { return mFabricTable; }
    PersistentStorageDelegate &GetPersistentStorage() { return mPersistentStorage; }
    InMemoryPersistentStorageDelegate &GetInMemoryStorage() { return mPersistentStorage; }
    CommissioningWindowManager &GetCommissioningWindowManager() { return mCommissioningMgr; }

    System::Clock::Milliseconds64 TimeSinceInit() const { return mTimeSinceInit; }

    void SetTimeSinceInitForTest(System::Clock::Milliseconds64 ms) { mTimeSinceInit = ms; }

    void ResetForTest()
    {
        mFabricTable.SetFabricCountForTest(0);
        mCommissioningMgr.ResetForTest();
        mTimeSinceInit = System::Clock::Milliseconds64(1000);
    }

private:
    Server() = default;

    FabricTable mFabricTable;
    InMemoryPersistentStorageDelegate mPersistentStorage;
    CommissioningWindowManager mCommissioningMgr;
    System::Clock::Milliseconds64 mTimeSinceInit{1000};
};

} // namespace chip

// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace esp_matter {

inline int &FactoryResetCallCountForTest()
{
    static int count = 0;
    return count;
}

inline void factory_reset()
{
    ++FactoryResetCallCountForTest();
}

inline void ResetFactoryResetCallCountForTest()
{
    FactoryResetCallCountForTest() = 0;
}

} // namespace esp_matter

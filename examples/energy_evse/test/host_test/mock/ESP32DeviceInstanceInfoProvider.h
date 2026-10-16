// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "chip_support.h"

namespace chip {
namespace DeviceLayer {

class DeviceInstanceInfoProvider {
public:
    virtual ~DeviceInstanceInfoProvider() = default;
};

class EvseDeviceInstanceInfoProvider : public DeviceInstanceInfoProvider {
public:
    CHIP_ERROR GetVendorName(char *buf, size_t bufSize);
    CHIP_ERROR GetVendorId(uint16_t &vendorId);
    CHIP_ERROR GetProductName(char *buf, size_t bufSize);
    CHIP_ERROR GetProductId(uint16_t &productId);
    CHIP_ERROR GetPartNumber(char *buf, size_t bufSize);
    CHIP_ERROR GetProductURL(char *buf, size_t bufSize);
    CHIP_ERROR GetProductLabel(char *buf, size_t bufSize);
    CHIP_ERROR GetSerialNumber(char *buf, size_t bufSize);
    CHIP_ERROR GetManufacturingDate(uint16_t &year, uint8_t &month, uint8_t &day);
    CHIP_ERROR GetHardwareVersion(uint16_t &hardwareVersion);
    CHIP_ERROR GetHardwareVersionString(char *buf, size_t bufSize);
    CHIP_ERROR GetRotatingDeviceIdUniqueId(MutableByteSpan &uniqueIdSpan);
};

} // namespace DeviceLayer
} // namespace chip

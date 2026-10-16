// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include "helpers.h"

#ifdef UNIT_TEST
#include "chip_support.h"
#include "chip_device_layer.h"
#include "esp_mac.h"
#include "ESP32DeviceInstanceInfoProvider.h"
#else
#include "esp_mac.h"
#include <lib/support/CHIPMemString.h>
#include <platform/CHIPDeviceLayer.h>
#include <string>
#include <platform/internal/GenericDeviceInstanceInfoProvider.h>
#include "ESP32DeviceInstanceInfoProvider.h"
#endif

namespace {
constexpr const char kDeviceVendorName[] = "ElaadNL";
constexpr const char kDeviceProductName[] = "Matter EVSE";
constexpr const char kDeviceHardwareVersionString[] = "EVSE-SIM-1.0";
} // namespace

namespace chip {
namespace DeviceLayer {

CHIP_ERROR EvseDeviceInstanceInfoProvider::GetVendorName(char *buf, size_t bufSize)
{
    ReturnErrorCodeIf(bufSize < sizeof(kDeviceVendorName), CHIP_ERROR_BUFFER_TOO_SMALL);
    Platform::CopyString(buf, bufSize, kDeviceVendorName);
    return CHIP_NO_ERROR;
}

CHIP_ERROR EvseDeviceInstanceInfoProvider::GetVendorId(uint16_t &vendorId)
{
    vendorId = static_cast<uint16_t>(CHIP_DEVICE_CONFIG_DEVICE_VENDOR_ID);
    return CHIP_NO_ERROR;
}

CHIP_ERROR EvseDeviceInstanceInfoProvider::GetProductName(char *buf, size_t bufSize)
{
    ReturnErrorCodeIf(bufSize < sizeof(kDeviceProductName), CHIP_ERROR_BUFFER_TOO_SMALL);
    Platform::CopyString(buf, bufSize, kDeviceProductName);

    return CHIP_NO_ERROR;
}

CHIP_ERROR EvseDeviceInstanceInfoProvider::GetProductId(uint16_t &productId)
{
    productId = static_cast<uint16_t>(CHIP_DEVICE_CONFIG_DEVICE_PRODUCT_ID);
    return CHIP_NO_ERROR;
}

CHIP_ERROR EvseDeviceInstanceInfoProvider::GetPartNumber(char *buf, size_t bufSize)
{
    (void)buf;
    (void)bufSize;
    // PartNumber is unsupported. Do not attach side effects (e.g. factory reset) to
    // attribute reads — use the explicit console command `sw factoryreset` instead.
    return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
}

CHIP_ERROR EvseDeviceInstanceInfoProvider::GetProductURL(char *buf, size_t bufSize)
{
    return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
}

CHIP_ERROR EvseDeviceInstanceInfoProvider::GetProductLabel(char *buf, size_t bufSize)
{
    return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
}

CHIP_ERROR EvseDeviceInstanceInfoProvider::GetSerialNumber(char *buf, size_t bufSize)
{
    uint8_t mac[6];
    // Use the Base MAC (or WIFI_STA) as the unique identifier
    esp_err_t err = esp_read_mac(mac, ESP_MAC_WIFI_STA);
    if (err != ESP_OK) {
#ifndef UNIT_TEST
        ChipLogError(DeviceLayer, "Failed to read MAC address: %d", err);
#endif
        return CHIP_ERROR_INTERNAL;
    }

    // Format the string: EVSE-SIM-XXXXXX (6 hex chars from last 3 bytes of MAC)
    // snprintf returns the number of characters that would have been written
    int written = snprintf(buf, bufSize, "EVSE-SIM-%02X%02X%02X", mac[3], mac[4], mac[5]);

    // Check if the buffer was large enough
    if (written < 0 || static_cast<size_t>(written) >= bufSize) {
#ifndef UNIT_TEST
        ChipLogError(DeviceLayer, "Serial number buffer too small (needed %d, got %u)", written + 1,
                     (unsigned int)bufSize);
#endif
        return CHIP_ERROR_BUFFER_TOO_SMALL;
    }

    PRINTF_DEBUG("Device Serial Number set to: %s", buf);

    return CHIP_NO_ERROR;
}

CHIP_ERROR EvseDeviceInstanceInfoProvider::GetManufacturingDate(uint16_t &year, uint8_t &month, uint8_t &day)
{
    const std::string dateStr = "2026-01-23";
    char *parseEnd;

    // Cast does not lose information, because we then check that we only parsed
    // 4 digits, so our number can't be bigger than 9999.
    year = static_cast<uint16_t>(strtoul(dateStr.c_str(), &parseEnd, 10));

    // Cast does not lose information, because we then check that we only parsed
    // 2 digits, so our number can't be bigger than 99.
    month = static_cast<uint8_t>(strtoul(dateStr.c_str() + 5, &parseEnd, 10));

    // Cast does not lose information, because we then check that we only parsed
    // 2 digits, so our number can't be bigger than 99.
    day = static_cast<uint8_t>(strtoul(dateStr.c_str() + 8, &parseEnd, 10));

    return CHIP_NO_ERROR;
}

CHIP_ERROR EvseDeviceInstanceInfoProvider::GetHardwareVersion(uint16_t &hardwareVersion)
{
    return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
}

CHIP_ERROR EvseDeviceInstanceInfoProvider::GetHardwareVersionString(char *buf, size_t bufSize)
{
    ReturnErrorCodeIf(bufSize < sizeof(kDeviceHardwareVersionString), CHIP_ERROR_BUFFER_TOO_SMALL);
    Platform::CopyString(buf, bufSize, kDeviceHardwareVersionString);
    return CHIP_NO_ERROR;
}

CHIP_ERROR EvseDeviceInstanceInfoProvider::GetRotatingDeviceIdUniqueId(MutableByteSpan &uniqueIdSpan)
{
    return CHIP_ERROR_WRONG_KEY_TYPE;
}

} // namespace DeviceLayer
} // namespace chip
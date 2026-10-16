// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "device.h"
#include "esp_err.h"

#include <cstdint>

struct esp_matter_attr_val_t {
    struct Val {
        int64_t i64 = 0;
        uint32_t u32 = 0;
        void *p = nullptr;
        struct {
            uint8_t *b = nullptr;
        } a;
    } val;
};

inline esp_matter_attr_val_t esp_matter_uint32(uint32_t v)
{
    esp_matter_attr_val_t out{};
    out.val.u32 = v;
    return out;
}

inline esp_matter_attr_val_t esp_matter_char_str(char *data, size_t len)
{
    esp_matter_attr_val_t out{};
    out.val.a.b = reinterpret_cast<uint8_t *>(data);
    (void)len;
    return out;
}

namespace esp_matter {

struct node_t {
};
struct endpoint_t {
};
struct cluster_t {
};

namespace identification {

enum callback_type_t : int { IDENTIFY = 0 };

using callback_t = esp_err_t (*)(callback_type_t, uint16_t, uint8_t, uint8_t, void *);

} // namespace identification

namespace attribute {

enum callback_type_t : int {
    PRE_UPDATE = 0,
    POST_UPDATE = 1,
    READ = 2,
    WRITE = 3,
};

using callback_t = esp_err_t (*)(callback_type_t, uint16_t, uint32_t, uint32_t, esp_matter_attr_val_t *, void *);

inline esp_err_t report(uint16_t, uint32_t, uint32_t, esp_matter_attr_val_t *)
{
    return ESP_OK;
}

} // namespace attribute

namespace cluster {

inline cluster_t *get(uint16_t, uint32_t)
{
    return nullptr;
}

namespace energy_evse {
namespace attribute {
inline void create_user_maximum_charge_current(cluster_t *, int64_t)
{
}
} // namespace attribute
namespace feature {
namespace charging_preferences {
inline void add(cluster_t *)
{
}
} // namespace charging_preferences
namespace rfid {
inline void add(cluster_t *)
{
}
} // namespace rfid
namespace soc_reporting {
inline void add(cluster_t *)
{
}
} // namespace soc_reporting
namespace v2x {
inline void add(cluster_t *)
{
}
} // namespace v2x
} // namespace feature
} // namespace energy_evse

namespace power_source {
namespace attribute {
inline void create_description(cluster_t *, const char *, size_t)
{
}
inline void create_wired_nominal_voltage(cluster_t *, uint32_t, uint32_t, uint32_t)
{
}
inline void create_wired_maximum_current(cluster_t *, uint32_t, uint32_t, uint32_t)
{
}
} // namespace attribute
namespace feature {
namespace wired {
inline void add(cluster_t *, void *)
{
}
} // namespace wired
namespace battery {
inline void add(cluster_t *, void *)
{
}
} // namespace battery
namespace rechargeable {
inline void add(cluster_t *, void *)
{
}
} // namespace rechargeable
namespace replaceable {
inline void add(cluster_t *, void *)
{
}
} // namespace replaceable
} // namespace feature
} // namespace power_source

namespace device_energy_management::feature::power_adjustment {
inline void add(cluster_t *)
{
}
} // namespace device_energy_management::feature::power_adjustment

} // namespace cluster

namespace node {
struct config_t {
};
inline node_t *create(config_t *, attribute::callback_t, identification::callback_t)
{
    static node_t node;
    return &node;
}
} // namespace node

constexpr int ENDPOINT_FLAG_NONE = 0;

namespace endpoint {

inline uint16_t get_id(endpoint_t *)
{
    static uint16_t next = 1;
    return next++;
}

namespace energy_evse {
struct config_t {
    struct {
        void *delegate = nullptr;
    } energy_evse;
    struct {
        void *delegate = nullptr;
    } energy_evse_mode;
};
inline endpoint_t *create(node_t *, config_t *, int, void *)
{
    static endpoint_t ep;
    return &ep;
}
} // namespace energy_evse

namespace power_source_device {
struct config_t {
};
inline endpoint_t *create(node_t *, config_t *, int, void *)
{
    static endpoint_t ep;
    return &ep;
}
} // namespace power_source_device

namespace electrical_sensor {
struct config_t {
    struct {
        void *delegate = nullptr;
    } power_topology;
    struct {
        void *delegate = nullptr;
    } electrical_power_measurement;
};
inline endpoint_t *create(node_t *, config_t *, int, void *)
{
    static endpoint_t ep;
    return &ep;
}
} // namespace electrical_sensor

namespace device_energy_management {
struct config_t {
    struct {
        void *delegate = nullptr;
    } device_energy_management;
};
inline endpoint_t *create(node_t *, config_t *, int, void *)
{
    static endpoint_t ep;
    return &ep;
}
} // namespace device_energy_management

} // namespace endpoint

inline esp_err_t &MatterStartReturnForTest()
{
    static esp_err_t err = ESP_OK;
    return err;
}

inline esp_err_t start(void (*)(const chip::DeviceLayer::ChipDeviceEvent *, intptr_t), intptr_t)
{
    return MatterStartReturnForTest();
}

} // namespace esp_matter

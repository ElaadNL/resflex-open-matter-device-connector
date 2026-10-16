// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "chip_support.h"

namespace chip {
using EndpointId = uint16_t;

namespace app {
namespace Clusters {
namespace PowerTopology {

enum class Feature : uint32_t {
    kSetTopology = 0x01,
    kDynamicPowerFlow = 0x02,
};

class Delegate {
public:
    virtual ~Delegate() = default;
    virtual CHIP_ERROR GetAvailableEndpointAtIndex(size_t index, EndpointId &endpointId) = 0;
    virtual CHIP_ERROR GetActiveEndpointAtIndex(size_t index, EndpointId &endpointId) = 0;
};

class PowerTopologyDelegate : public Delegate {
public:
    ~PowerTopologyDelegate() override = default;

    CHIP_ERROR GetAvailableEndpointAtIndex(size_t index, EndpointId &endpointId) override;
    CHIP_ERROR GetActiveEndpointAtIndex(size_t index, EndpointId &endpointId) override;

    void SetupDelegate(EndpointId id);
    void SetAvailableEndpointIds(EndpointId id, size_t index);
    void AddCustomAttributes() const;
    void AddCustomFeatures(Feature aFeature);
    void LateSetupAfterMatter();

private:
    EndpointId mEndpointId = 0;
    EndpointId mAvailableEps[1] = {};
    uint32_t mFeatureRaw = 0;
};

class Instance {
public:
    static CHIP_ERROR Init() { return CHIP_NO_ERROR; }
    static void Shutdown() {}
};

class PowerTopologyInstance : public Instance {
public:
    PowerTopologyInstance(EndpointId, PowerTopologyDelegate &delegate, Feature, int) { mDelegate = &delegate; }

    CHIP_ERROR InitializeCluster();
    void ShutdownCluster();
    PowerTopologyDelegate *GetDelegate();

private:
    PowerTopologyDelegate *mDelegate = nullptr;
};

} // namespace PowerTopology
} // namespace Clusters
} // namespace app
} // namespace chip

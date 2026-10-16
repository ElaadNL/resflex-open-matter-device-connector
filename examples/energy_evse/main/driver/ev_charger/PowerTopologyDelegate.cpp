/*
 *
 *    Copyright (c) 2024 Project CHIP Authors
 *    All rights reserved.
 *    SPDX-FileCopyrightText: ElaadNL
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "helpers.h"

#ifdef UNIT_TEST
#include "PowerTopologyDelegate_host.h"
#else
#include "esp_matter.h"
#include <PowerTopologyDelegate.h>
#endif

#include <iterator>

#ifdef UNIT_TEST
#include "chip_support.h"
#endif

using namespace chip;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::PowerTopology;

#ifndef UNIT_TEST
using namespace esp_matter;
#endif

CHIP_ERROR PowerTopologyDelegate::GetAvailableEndpointAtIndex(size_t index, EndpointId &endpointId)
{
    PRINTF_DEBUG("GetAvailableEndpointAtIndex:%d", index);
    if (index >= std::size(mAvailableEps))
        return CHIP_ERROR_PROVIDER_LIST_EXHAUSTED;

    if (index == 0) {
        endpointId = mAvailableEps[index];
        PRINTF_DEBUG("GetAvailableEndpointAtIndex:%d = %d", index, endpointId);
        return CHIP_NO_ERROR;
    }

    return CHIP_ERROR_NOT_FOUND;
}

CHIP_ERROR PowerTopologyDelegate::GetActiveEndpointAtIndex(size_t index, EndpointId &endpointId)
{
    PRINTF_DEBUG("GetActiveEndpointAtIndex:%d", index);
    return CHIP_ERROR_PROVIDER_LIST_EXHAUSTED;
}

void PowerTopologyDelegate::SetupDelegate(EndpointId id)
{
    mEndpointId = id;

    AddCustomAttributes();
}

void PowerTopologyDelegate::SetAvailableEndpointIds(EndpointId id, size_t index)
{
    if (index >= std::size(mAvailableEps))
        return;

    mAvailableEps[index] = id;

    PRINTF_DEBUG("Set Available Endpoints Idx:%d=%d", index, id);
}

void PowerTopologyDelegate::AddCustomAttributes() const
{
    /*
     * Empty: PowerTopology custom attributes are provisioned when the cluster is created;
     * nothing is added here for this product variant.
     */
}

void PowerTopologyDelegate::AddCustomFeatures(Feature aFeature)
{
#ifndef UNIT_TEST
    mFeature.Set(aFeature);

    // this is already set when create the power_topology cluster in esp_matter_endpoint.cpp
    if (mFeature.Has(Feature::kSetTopology)) {
        using namespace esp_matter::cluster::power_topology;
        if (mFeature.Has(Feature::kDynamicPowerFlow)) {
            esp_matter::cluster_t *cluster = esp_matter::cluster::get(mEndpointId, Clusters::PowerTopology::Id);
            feature::dynamic_power_flow::add(cluster);
        }
    }
#else
    mFeatureRaw = static_cast<uint32_t>(aFeature);
    (void)mEndpointId;
#endif
}

void PowerTopologyDelegate::LateSetupAfterMatter()
{
#ifndef UNIT_TEST
    PRINTF_DEBUG();
    esp_matter_attr_val_t val =
        esp_matter_array((uint8_t *)mAvailableEps, std::size(mAvailableEps), std::size(mAvailableEps));

    const esp_err_t err = esp_matter::attribute::report(mEndpointId, PowerTopology::Id,
                                                        PowerTopology::Attributes::AvailableEndpoints::Id, &val);
    if (err != ESP_OK) {
        PRINTF_DEBUG("Power Source ActiveEndpoints report failed: %s", esp_err_to_name(err));
        return;
    }
    PRINTF_DEBUG("Power Source ActiveEndpoints: (OK)");
#else
    (void)mAvailableEps;
#endif
}

CHIP_ERROR PowerTopologyInstance::InitializeCluster()
{
    return Instance::Init();
}

void PowerTopologyInstance::ShutdownCluster()
{
    Instance::Shutdown();
}

PowerTopologyDelegate *PowerTopologyInstance::GetDelegate()
{
    return mDelegate;
}

// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include "PowerTopologyDelegate_host.h"

#include "unity.h"

using namespace chip::app::Clusters::PowerTopology;

static void test_power_topology_delegate_constructible(void)
{
    PowerTopologyDelegate delegate;
    delegate.AddCustomAttributes();
    delegate.AddCustomFeatures(Feature::kSetTopology);
    delegate.LateSetupAfterMatter();
}

void run_test_power_topology_delegate_header_tests(void)
{
    RUN_TEST(test_power_topology_delegate_constructible);
}

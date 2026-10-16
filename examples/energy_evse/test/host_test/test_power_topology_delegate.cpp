// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include "PowerTopologyDelegate_host.h"

#include "unity.h"

using namespace chip;
using namespace chip::app::Clusters::PowerTopology;

static void test_available_endpoint_index_zero(void)
{
    PowerTopologyDelegate delegate;
    delegate.SetupDelegate(1);
    delegate.SetAvailableEndpointIds(9, 0);

    EndpointId ep = 0;
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.GetAvailableEndpointAtIndex(0, ep));
    TEST_ASSERT_EQUAL(9, ep);
}

static void test_available_endpoint_exhausted(void)
{
    PowerTopologyDelegate delegate;
    EndpointId ep = 0;
    TEST_ASSERT_EQUAL(CHIP_ERROR_PROVIDER_LIST_EXHAUSTED, delegate.GetAvailableEndpointAtIndex(1, ep));
}

static void test_active_endpoint_exhausted(void)
{
    PowerTopologyDelegate delegate;
    EndpointId ep = 0;
    TEST_ASSERT_EQUAL(CHIP_ERROR_PROVIDER_LIST_EXHAUSTED, delegate.GetActiveEndpointAtIndex(0, ep));
}

static void test_instance_get_delegate_init_shutdown(void)
{
    PowerTopologyDelegate delegate;
    PowerTopologyInstance instance(2, delegate, Feature::kSetTopology, 0);

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, instance.InitializeCluster());
    TEST_ASSERT_EQUAL_PTR(&delegate, instance.GetDelegate());
    instance.ShutdownCluster();
}

void run_test_power_topology_delegate_tests(void)
{
    RUN_TEST(test_available_endpoint_index_zero);
    RUN_TEST(test_available_endpoint_exhausted);
    RUN_TEST(test_active_endpoint_exhausted);
    RUN_TEST(test_instance_get_delegate_init_shutdown);
}
